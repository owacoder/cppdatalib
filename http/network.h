/*
 * network.h
 *
 * Copyright © 2018 Oliver Adams
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Disclaimer:
 * Trademarked product names referred to in this file are the property of their
 * respective owners. These trademark owners are not affiliated with the author
 * or copyright holder(s) of this file in any capacity, and do not endorse this
 * software nor the authorship and existence of this file.
 */

#ifndef NETWORK_H
#define NETWORK_H

#include "../core/core.h"

#ifdef CPPDATALIB_ENABLE_QT_NETWORK
#include <QtNetwork/QtNetwork>
#endif

#ifdef CPPDATALIB_ENABLE_POCO_NETWORK
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/URI.h>
#endif

#ifdef CPPDATALIB_ENABLE_CURL_NETWORK
#include <curl/curl.h>
#endif

namespace cppdatalib
{
    namespace http
    {
#ifdef CPPDATALIB_ENABLE_QT_NETWORK
        class qt_parser : public core::stream_input
        {
            core::value url;
            core::object_t headers;
            std::string verb;
            int maximum_redirects;
            core::value proxy_settings;
            QNetworkAccessManager *manager;
            bool owns_manager;

            QNetworkRequest request;
            QNetworkReply *reply;

        public:
            qt_parser(const core::value &url /* URL may include headers as attributes if CPPDATALIB_ENABLE_ATTRIBUTES is defined */,
                      const std::string &verb = "GET",
                      const core::object_t &headers = core::object_t(),
                      int max_redirects = 20,
                      const core::object_t &proxy_settings = core::object_t(),
                      QNetworkAccessManager *manager = nullptr)
                : url(url)
                , headers(headers)
                , verb(verb)
                , maximum_redirects(max_redirects)
                , proxy_settings(proxy_settings)
                , manager(manager? manager: new QNetworkAccessManager())
                , owns_manager(manager == nullptr)
                , reply(nullptr)
            {
                reset();
            }
            ~qt_parser()
            {
                if (reply)
                    reply->deleteLater();
                if (owns_manager)
                    delete manager;
            }

            bool busy() const {return stream_input::busy() || reply;}

        protected:
            void reset_()
            {
                request.setUrl(QUrl(QString::fromStdString(url.as_string())));
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                for (const auto &header: url.get_attributes())
                    request.setRawHeader(QByteArray::fromStdString(header.first.as_string()),
                                         QByteArray::fromStdString(header.second.as_string()));
#endif
                for (const auto &header: headers)
                    request.setRawHeader(QByteArray::fromStdString(header.first.as_string()),
                                         QByteArray::fromStdString(header.second.as_string()));
                request.setMaximumRedirectsAllowed(maximum_redirects);
                request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, (maximum_redirects >= 0));

                if (proxy_settings.object_size())
                    manager->setProxy(QNetworkProxy(QNetworkProxy::HttpProxy,
                                                    QString::fromStdString(proxy_settings.const_member("host").as_string()),
                                                    proxy_settings.const_member("port").as_uint(),
                                                    QString::fromStdString(proxy_settings.const_member("username").as_string()),
                                                    QString::fromStdString(proxy_settings.const_member("password").as_string())));
                else
                    manager->setProxy(QNetworkProxy());

                if (reply)
                    reply->deleteLater();
                reply = nullptr;
            }

            void write_one_()
            {
                core::value string_type(core::string_t(), core::blob);

#ifndef CPPDATALIB_EVENT_LOOP_CALLBACK
                qApp->processEvents();
                QThread::usleep(100);
#endif

                if (was_just_reset())
                {
                    reply = manager->sendCustomRequest(request, QByteArray::fromStdString(verb));
                    return;
                }

                if (reply->error() != QNetworkReply::NoError)
                    throw core::custom_error("HTTP - " + reply->errorString().toStdString());

                if (get_output()->current_container() != core::string)
                {
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                    for (const auto &header: reply->rawHeaderPairs())
                        string_type.add_attribute_at_end(core::value(header.first.toStdString()), core::value(header.second.toStdString()));
#endif

                    get_output()->begin_string(string_type, core::stream_handler::unknown_size());
                }

                if (reply->isFinished())
                {
                    if (reply->bytesAvailable() > 0)
                        get_output()->append_to_string(core::value(reply->readAll().toStdString()));
                    get_output()->end_string(string_type);

                    reply->deleteLater(); reply = nullptr;
                }
                else if (reply->bytesAvailable() > 0)
                    get_output()->append_to_string(core::value(reply->readAll().toStdString()));
            }
        };
#endif

#ifdef CPPDATALIB_ENABLE_POCO_NETWORK
        class poco_parser : public core::stream_input
        {
            std::unique_ptr<char []> buffer;

            core::value url, working_url;
            core::object_t headers;
            std::string verb;
            int maximum_redirects;
            int redirects;
            core::value proxy_settings;
            Poco::Net::HTTPClientSession *http;
            Poco::Net::HTTPSClientSession *https;
            bool owns_http_session, owns_https_session, is_https;

            Poco::Net::HTTPRequest request;
            Poco::Net::HTTPResponse response;

        public:
            poco_parser(const core::value &url /* URL may include headers as attributes if CPPDATALIB_ENABLE_ATTRIBUTES is defined */,
                        const std::string &verb = "GET",
                        const core::object_t &headers = core::object_t(),
                        int max_redirects = 20,
                        const core::object_t &proxy_settings = core::object_t(),
                        Poco::Net::HTTPClientSession *session = nullptr,
                        Poco::Net::HTTPSClientSession *s_session = nullptr)
                : buffer(new char [core::buffer_size])
                , url(url)
                , headers(headers)
                , verb(verb)
                , maximum_redirects(max_redirects)
                , redirects(0)
                , proxy_settings(proxy_settings)
                , http(session)
                , https(s_session)
                , owns_http_session(false)
                , owns_https_session(false)
                , is_https(false)
            {
                reset();
            }
            ~poco_parser()
            {
                if (owns_http_session)
                    delete http;
                if (owns_https_session)
                    delete https;
            }

            bool busy() const {return stream_input::busy() || redirects;}

        protected:
            void init_to_url(const std::string &new_url)
            {
                try
                {
                    working_url.set_string(new_url);

                    Poco::URI uri(working_url.as_string());
                    std::string path_part(uri.getPathAndQuery());

                    request.setMethod(verb);
                    request.setURI(path_part.empty()? "/": path_part);
                    request.setVersion(Poco::Net::HTTPMessage::HTTP_1_1);

#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                    for (const auto &header: working_url.get_attributes())
                        request.add(header.first.as_string(),
                                    header.second.as_string());
#endif
                    for (const auto &header: headers)
                        request.add(header.first.as_string(),
                                    header.second.as_string());

                    if (uri.getScheme() == "https")
                    {
                        is_https = true;
                        if (https == nullptr)
                        {
                            https = new Poco::Net::HTTPSClientSession(uri.getHost(), uri.getPort()? uri.getPort(): unsigned(Poco::Net::HTTPSClientSession::HTTPS_PORT));
                            owns_https_session = true;
                        }

                        if (uri.getHost() != https->getHost() ||
                            (uri.getPort() != https->getPort() && uri.getPort() != 0))
                        {
                            https->reset();
                            https->setHost(uri.getHost());
                            https->setPort(uri.getPort());
                            https->setKeepAlive(true);
                        }

                        https->setProxy(proxy_settings.const_member("host").get_string(),
                                        proxy_settings.const_member("port").as_uint(Poco::Net::HTTPSession::HTTP_PORT));
                        https->setProxyCredentials(proxy_settings.const_member("username").get_string(),
                                                   proxy_settings.const_member("password").get_string());
                    }
                    else if (uri.getScheme() == "http")
                    {
                        is_https = false;
                        if (http == nullptr)
                        {
                            http = new Poco::Net::HTTPClientSession(uri.getHost(), uri.getPort()? uri.getPort(): unsigned(Poco::Net::HTTPClientSession::HTTP_PORT));
                            owns_http_session = true;
                        }

                        if (uri.getHost() != http->getHost() ||
                            (uri.getPort() != http->getPort() && uri.getPort() != 0))
                        {
                            http->reset();
                            http->setHost(uri.getHost());
                            http->setPort(uri.getPort());
                            http->setKeepAlive(true);
                        }

                        http->setProxy(proxy_settings.const_member("host").get_string(),
                                       proxy_settings.const_member("port").as_uint(Poco::Net::HTTPSession::HTTP_PORT));
                        http->setProxyCredentials(proxy_settings.const_member("username").get_string(),
                                                  proxy_settings.const_member("password").get_string());
                    }
                    else
                        throw core::custom_error("HTTP - invalid scheme \"" + uri.getScheme() + "\" requested in URL");
                } catch (const Poco::Exception &e) {
                    throw core::custom_error("HTTP - " + std::string(e.what()));
                }
            }

            void reset_()
            {
                working_url = url;
                redirects = 0;

                init_to_url(url.as_string());
            }

            void write_one_()
            {
                try
                {
                    core::value string_type(core::string_t(), core::blob);
                    std::istream *in;

                    if (is_https)
                    {
                        https->sendRequest(request);

                        in = &https->receiveResponse(response);
                    }
                    else
                    {
                        http->sendRequest(request);

                        in = &http->receiveResponse(response);
                    }

                    if (response.getStatus() / 100 >= 4)
                        throw core::custom_error("HTTP - error \"" + response.getReason() + "\" while retrieving \"" + working_url.as_string() + "\"");
                    else if (response.getStatus() / 100 == 3)
                    {
                        if (++redirects > maximum_redirects && maximum_redirects >= 0)
                            throw core::custom_error("HTTP - request to \"" + url.as_string() + "\" failed with too many redirects");

                        if (!response.has("location"))
                            throw core::custom_error("HTTP - redirection from \"" + working_url.as_string() + "\" has no destination location");

                        init_to_url(response.get("location"));
                        return;
                    }
                    else
                    {
                        redirects = 0;
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                        for (const auto &header: response)
                            string_type.add_attribute_at_end(core::value(header.first), core::value(header.second));
#endif
                    }

                    get_output()->begin_string(string_type, core::stream_handler::unknown_size());
                    while (true)
                    {
                        in->read(buffer.get(), core::buffer_size);

                        if (*in)
                            get_output()->append_to_string(core::value(core::string_t(buffer.get(), core::buffer_size)));
                        else if (in->eof())
                        {
                            get_output()->append_to_string(core::value(core::string_t(buffer.get(), in->gcount())));
                            get_output()->end_string(string_type);
                            break;
                        }
                        else
                            throw core::custom_error("HTTP - an error occured while retrieving \"" + working_url.as_string() + "\"");
                    }
                } catch (const Poco::Exception &e) {
                    throw core::custom_error("HTTP - " + std::string(e.what()));
                }
            }
        };
#endif

#ifdef CPPDATALIB_ENABLE_CURL_NETWORK
        class curl_parser : public core::stream_input
        {
            std::unique_ptr<char []> buffer;

            core::value url, working_url;
            core::object_t headers;
            std::string verb;
            int maximum_redirects;
            int redirects;
            core::value proxy_settings;
            CURL *easy;
            CURLM *curl;
            bool owns_easy, owns_curl;

        public:
            curl_parser(const core::value &url /* URL may include headers as attributes if CPPDATALIB_ENABLE_ATTRIBUTES is defined */,
                        const std::string &verb = "GET",
                        const core::object_t &headers = core::object_t(),
                        int max_redirects = 20,
                        const core::object_t &proxy_settings = core::object_t(),
                        CURL *connection = nullptr,
                        CURLM *session = nullptr)
                : buffer(new char [core::buffer_size])
                , url(url)
                , headers(headers)
                , verb(verb)
                , maximum_redirects(max_redirects)
                , redirects(0)
                , proxy_settings(proxy_settings)
                , easy(connection)
                , curl(session)
                , owns_easy(false)
                , owns_curl(false)
            {
                reset();
            }
            ~curl_parser()
            {
                if (owns_easy)
                    curl_easy_cleanup(easy);
                if (owns_curl)
                    curl_multi_cleanup(curl);
            }

            bool busy() const {return stream_input::busy() || redirects;}

        protected:
            void init_to_url(const std::string &new_url)
            {
                try
                {
                    if (!curl)
                    {
                        curl = curl_multi_init();
                        if (!curl)
                            throw std::bad_alloc();
                        owns_curl = true;
                    }

                    if (!easy)
                    {
                        easy = curl_easy_init();
                        if (!easy)
                            throw std::bad_alloc();
                        owns_easy = true;
                    }

                    curl_easy_reset(easy);
                    curl_easy_setopt(easy, CURLOPT_URL, working_url.as_string().c_str());
                    if (proxy_settings.object_size())
                    {
                        curl_easy_setopt(easy, CURLOPT_PROXYTYPE, working_url.as_string().substr(0, 5) == "https"? CURLPROXY_HTTPS: CURLPROXY_HTTP);
                        curl_easy_setopt(easy, CURLOPT_PROXY, proxy_settings.const_member("host").get_cstring());
                        curl_easy_setopt(easy, CURLOPT_PROXYPORT, proxy_settings.const_member("port").as_uint(80));
                        curl_easy_setopt(easy, CURLOPT_PROXYUSERNAME, proxy_settings.const_member("username").get_cstring());
                        curl_easy_setopt(easy, CURLOPT_PROXYPASSWORD, proxy_settings.const_member("password").get_cstring());
                    }

                    working_url.set_string(new_url);

#if 0
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                    for (const auto &header: working_url.get_attributes())
                        request.add(header.first.as_string(),
                                    header.second.as_string());
#endif
                    for (const auto &header: headers)
                        request.add(header.first.as_string(),
                                    header.second.as_string());



                    if (uri.getScheme() == "https")
                    {
                        is_https = true;
                        if (https == nullptr)
                        {
                            https = new Poco::Net::HTTPSClientSession(uri.getHost(), uri.getPort()? uri.getPort(): unsigned(Poco::Net::HTTPSClientSession::HTTPS_PORT));
                            owns_https_session = true;
                        }

                        if (uri.getHost() != https->getHost() ||
                            (uri.getPort() != https->getPort() && uri.getPort() != 0))
                        {
                            https->reset();
                            https->setHost(uri.getHost());
                            https->setPort(uri.getPort());
                            https->setKeepAlive(true);
                        }

                        https->setProxy(proxy_settings.const_member("host").get_string(),
                                        proxy_settings.const_member("port").as_uint(Poco::Net::HTTPSession::HTTP_PORT));
                        https->setProxyCredentials(proxy_settings.const_member("username").get_string(),
                                                   proxy_settings.const_member("password").get_string());
                    }
                    else if (uri.getScheme() == "http")
                    {
                        is_https = false;
                        if (http == nullptr)
                        {
                            http = new Poco::Net::HTTPClientSession(uri.getHost(), uri.getPort()? uri.getPort(): unsigned(Poco::Net::HTTPClientSession::HTTP_PORT));
                            owns_http_session = true;
                        }

                        if (uri.getHost() != http->getHost() ||
                            (uri.getPort() != http->getPort() && uri.getPort() != 0))
                        {
                            http->reset();
                            http->setHost(uri.getHost());
                            http->setPort(uri.getPort());
                            http->setKeepAlive(true);
                        }

                        http->setProxy(proxy_settings.const_member("host").get_string(),
                                       proxy_settings.const_member("port").as_uint(Poco::Net::HTTPSession::HTTP_PORT));
                        http->setProxyCredentials(proxy_settings.const_member("username").get_string(),
                                                  proxy_settings.const_member("password").get_string());
                    }
                    else
                        throw core::custom_error("HTTP - invalid scheme \"" + uri.getScheme() + "\" requested in URL");
#endif
                } catch (const Poco::Exception &e) {
                    throw core::custom_error("HTTP - " + std::string(e.what()));
                }
            }

            void reset_()
            {
                working_url = url;
                redirects = 0;

                init_to_url(url.as_string());
            }

            void write_one_()
            {
#if 0
                try
                {
                    core::value string_type(core::string_t(), core::blob);
                    std::istream *in;

                    if (is_https)
                    {
                        https->sendRequest(request);

                        in = &https->receiveResponse(response);
                    }
                    else
                    {
                        http->sendRequest(request);

                        in = &http->receiveResponse(response);
                    }

                    if (response.getStatus() / 100 >= 4)
                        throw core::custom_error("HTTP - error \"" + response.getReason() + "\" while retrieving \"" + working_url.as_string() + "\"");
                    else if (response.getStatus() / 100 == 3)
                    {
                        if (++redirects > maximum_redirects && maximum_redirects >= 0)
                            throw core::custom_error("HTTP - request to \"" + url.as_string() + "\" failed with too many redirects");

                        if (!response.has("location"))
                            throw core::custom_error("HTTP - redirection from \"" + working_url.as_string() + "\" has no destination location");

                        init_to_url(response.get("location"));
                        return;
                    }
                    else
                    {
                        redirects = 0;
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                        for (const auto &header: response)
                            string_type.add_attribute_at_end(core::value(header.first), core::value(header.second));
#endif
                    }

                    get_output()->begin_string(string_type, core::stream_handler::unknown_size());
                    while (true)
                    {
                        in->read(buffer.get(), core::buffer_size);

                        if (*in)
                            get_output()->append_to_string(core::value(core::string_t(buffer.get(), core::buffer_size)));
                        else if (in->eof())
                        {
                            get_output()->append_to_string(core::value(core::string_t(buffer.get(), in->gcount())));
                            get_output()->end_string(string_type);
                            break;
                        }
                        else
                            throw core::custom_error("HTTP - an error occured while retrieving \"" + working_url.as_string() + "\"");
                    }
                } catch (const Poco::Exception &e) {
                    throw core::custom_error("HTTP - " + std::string(e.what()));
                }
#endif
            }
        };
#endif

        inline void http_initialize()
        {

        }

        inline void http_deinitialize()
        {

        }

        class parser : public core::stream_input
        {
            core::value url;
            core::object_t headers;
            std::string verb;
            int maximum_redirects;
            core::object_t proxy_settings;
            core::network_library interface;
            core::stream_input *interface_stream;
            void *context, *s_context;

        public:
            parser(const core::value &url /* URL may include headers as attributes if CPPDATALIB_ENABLE_ATTRIBUTES is defined */,
                   core::network_library interface = core::default_network_library,
                   const std::string &verb = "GET",
                   const core::object_t &headers = core::object_t(),
                   int max_redirects = 20,
                   /*
                    *  proxy_settings:
                    *  {
                    *      "host": <host_name> (string),
                    *      "port": <port> (uinteger),
                    *      "username": <username> (string),
                    *      "password": <password> (string)
                    *  }
                    */
                   const core::object_t &proxy_settings = core::object_t(),
                   void *context = nullptr /* Context varies, based on individual interfaces (e.g. qt_parser takes a pointer to a QNetworkAccessManager) */,
                   void *s_context = nullptr /* Secondary context varies, based on individual interfaces (e.g. poco_parser takes a pointer to Poco::Net::HTTPClientSession as initial context, and a pointer to Poco::Net::HTTPSClientSession as secondary context) */)
                : url(url)
                , headers(headers)
                , verb(verb)
                , maximum_redirects(max_redirects)
                , proxy_settings(proxy_settings)
                , interface(core::unknown_network_library)
                , interface_stream(nullptr)
                , context(context)
                , s_context(s_context)
            {
                set_interface(interface);
                reset();
            }
            ~parser()
            {
                delete interface_stream;
            }

            void set_interface(core::network_library interface)
            {
                (void) context;
                (void) s_context;

                if (this->interface != interface)
                {
                    this->interface = interface;
                    delete interface_stream;

                    if (interface >= core::network_library_count)
                    {
                        interface_stream = nullptr;
                        return;
                    }

                    switch (interface)
                    {
                        case core::unknown_network_library:
                        default:
                            interface_stream = nullptr;
                            break;
#ifdef CPPDATALIB_ENABLE_QT_NETWORK
                        case core::qt_network_library:
                            interface_stream = new qt_parser(url, verb, headers, maximum_redirects, proxy_settings,
                                                             static_cast<QNetworkAccessManager *>(context));
                            if (get_output())
                                interface_stream->set_output(*get_output());
                            break;
#endif
#ifdef CPPDATALIB_ENABLE_POCO_NETWORK
                        case core::poco_network_library:
                            interface_stream = new poco_parser(url, verb, headers, maximum_redirects, proxy_settings,
                                                               static_cast<Poco::Net::HTTPClientSession *>(context),
                                                               static_cast<Poco::Net::HTTPSClientSession *>(s_context));
                            if (get_output())
                                interface_stream->set_output(*get_output());
                            break;
#endif
                    }

                    reset();
                }
            }
            core::network_library get_interface() const {return interface;}

            int max_redirects() const {return maximum_redirects;}
            void set_max_redirects(int max) {maximum_redirects = max;}

            bool busy() const {return stream_input::busy() || (interface_stream && interface_stream->busy());}

        protected:
            void test_interface()
            {
                if (interface_stream == nullptr)
                    throw core::error("HTTP - invalid, non-existent, or disabled network interface selected");
            }

            void output_changed_()
            {
                if (interface_stream != nullptr)
                    interface_stream->set_output(*get_output());
            }

            void reset_()
            {
                if (interface_stream != nullptr)
                    interface_stream->reset();
            }

            void write_one_()
            {
                test_interface();
                interface_stream->write_one();
            }
        };
    }
}

#endif // NETWORK_H
