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
        class http_error : public core::custom_error
        {
        public:
            http_error(int code, std::string error) : custom_error(error), code_(code) {}

            int response_code() const {return code_;}

        private:
            int code_;
        };

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

            core::istream_handle input_handle;
            QBuffer input_buffer;

        public:
            qt_parser(const core::value &url /* URL may include headers as attributes if CPPDATALIB_ENABLE_ATTRIBUTES is defined */,
                      const std::string &verb = "GET",
                      core::istream_handle input = core::istream_handle(),
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
                , input_handle(input)
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

                // Transfer as chunked, since we don't know precisely how long the input is
                if (input_handle.valid())
                    request.setRawHeader("Transfer-Encoding", "chunked");

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
                    if (input_handle.valid())
                    {
                        QByteArray data;
                        core::istream::int_type c;

                        while ((c = input_handle.stream().get(), c != EOF))
                            data += QByteArray::fromStdString(core::ucs_to_utf8(c));

                        input_buffer.close();
                        input_buffer.setData(data);
                        input_buffer.open(QIODevice::ReadOnly);
                        reply = manager->sendCustomRequest(request, QByteArray::fromStdString(verb), &input_buffer);
                    }
                    else
                        reply = manager->sendCustomRequest(request, QByteArray::fromStdString(verb));

                    return;
                }

#ifndef CPPDATALIB_ENABLE_ATTRIBUTES
                if (reply->error() != QNetworkReply::NoError)
                    throw http_error(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute), "HTTP - " + reply->errorString().toStdString());
#endif

                if ((reply->bytesAvailable() || reply->isFinished()) && get_output()->current_container() != core::string)
                {
                    core::value response_headers;

                    for (const auto &header: reply->rawHeaderPairs())
                        response_headers.add_member_at_end(core::value(core::ascii_lowercase_copy(header.first.toStdString())), core::value(core::ascii_trim_copy(header.second.toStdString())));

                    core::optional_size contentLength;

                    // Attempt to obtain HTTP response code
                    response_headers.member(core::value("")) = core::value(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toLongLong());

                    // Attempt to obtain content length
                    if (core::ascii_lowercase_copy(verb) != "head" &&
                            response_headers.is_member("content-length") &&
                            response_headers.const_member("transfer-encoding").get_string("identity") == "identity")
                        contentLength = core::optional_size(response_headers.const_member("content-length").as_uint());

                    // Attempt to obtain content type to adjust output string's type
                    if (response_headers.is_member("content-type"))
                    {
                        std::string contentType = core::ascii_lowercase_copy(response_headers.const_member("content-type").as_string());

                        if (contentType.substr(0, 5) == "text/")
                            string_type.set_subtype(contentType.find("charset=utf-8") != contentType.npos? core::normal: core::clob);
                    }

#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                    string_type.set_attributes(std::move(response_headers.get_object_ref()));
#endif

                    get_output()->begin_string(string_type, contentLength);
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

            core::istream_handle input_stream;

        public:
            poco_parser(const core::value &url /* URL may include headers as attributes if CPPDATALIB_ENABLE_ATTRIBUTES is defined */,
                        const std::string &verb = "GET",
                        core::istream_handle input = core::istream_handle(),
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
                , input_stream(input)
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
            void init_to_url(std::string new_url)
            {
                try
                {
                    working_url.set_string(new_url);

                    Poco::URI uri(working_url.as_string());
                    std::string path_part(uri.getPathAndQuery());

                    request.clear();
                    request.setMethod(verb);
                    request.setURI(path_part.empty()? "/": path_part);
                    request.setHost(uri.getHost());
                    request.setVersion(Poco::Net::HTTPMessage::HTTP_1_1);
                    if (!uri.getUserInfo().empty())
                    {
                        core::ostringstream out;
                        base64::encode_accumulator base64(out);
                        base64.accumulate(uri.getUserInfo());
                        base64.end();
                        request.setCredentials("Basic", out.str());
                    }

#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                    for (const auto &header: working_url.get_attributes())
                        request.add(header.first.as_string(),
                                    header.second.as_string());
#endif
                    for (const auto &header: headers)
                        request.add(header.first.as_string(),
                                    header.second.as_string());

                    // Transfer as chunked, since we don't know precisely how long the input is
                    if (input_stream.valid())
                        request.add("Transfer-Encoding", "chunked");

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
                            https->setProxy(proxy_settings.const_member("host").as_string(),
                                            proxy_settings.const_member("port").as_uint(Poco::Net::HTTPSession::HTTP_PORT));
                            https->setProxyCredentials(proxy_settings.const_member("username").as_string(),
                                                       proxy_settings.const_member("password").as_string());

                            https->setHost(uri.getHost());
                            https->setPort(uri.getPort());
                            https->setKeepAlive(true);
                        }
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
                            http->setProxy(proxy_settings.const_member("host").as_string(),
                                           proxy_settings.const_member("port").as_uint(Poco::Net::HTTPSession::HTTP_PORT));
                            http->setProxyCredentials(proxy_settings.const_member("username").as_string(),
                                                      proxy_settings.const_member("password").as_string());

                            http->setHost(uri.getHost());
                            http->setPort(uri.getPort());
                            http->setKeepAlive(true);
                        }
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
                    core::optional_size contentLength;
                    core::value string_type(core::string_t(), core::blob);
                    std::istream *in;

                    if (is_https)
                    {
                        std::ostream &out = https->sendRequest(request);

                        if (input_stream.valid())
                        {
                            core::istream::int_type c;

                            while ((c = input_stream.stream().get(), c != EOF))
                                out << core::ucs_to_utf8(c);
                        }

                        in = &https->receiveResponse(response);
                    }
                    else
                    {
                        std::ostream &out = http->sendRequest(request);

                        if (input_stream.valid())
                        {
                            core::istream::int_type c;

                            while ((c = input_stream.stream().get(), c != EOF))
                                out << core::ucs_to_utf8(c);
                        }

                        in = &http->receiveResponse(response);
                    }

#ifndef CPPDATALIB_ENABLE_ATTRIBUTES
                    if (response.getStatus() / 100 >= 4)
                        throw http_error(static_cast<int>(response.getStatus()), "HTTP - " + response.getReason());
                    else
#endif
                    if (response.getStatus() / 100 == 3)
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
                        core::value response_headers;

                        redirects = 0;
                        for (const auto &header: response)
                        {
                            std::string lcase = core::ascii_lowercase_copy(header.first);
                            std::string trimmed = core::ascii_trim_copy(header.second);

                            response_headers.member(core::value(lcase)) = core::value(trimmed);
                        }

                        // Attempt to obtain HTTP response code
                        response_headers.member(core::value("")) = core::value(static_cast<int>(response.getStatus()));

                        // Attempt to obtain content length
                        if (core::ascii_lowercase_copy(verb) != "head" &&
                                response_headers.is_member("content-length") &&
                                response_headers.const_member("transfer-encoding").get_string("identity") == "identity")
                            contentLength = core::optional_size(response_headers.const_member("content-length").as_uint());

                        // Attempt to obtain content type to adjust output string's type
                        if (response_headers.is_member("content-type"))
                        {
                            std::string contentType = core::ascii_lowercase_copy(response_headers.const_member("content-type").as_string());

                            if (contentType.substr(0, 5) == "text/")
                                string_type.set_subtype(contentType.find("charset=utf-8") != contentType.npos? core::normal: core::clob);
                        }

#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                        string_type->set_attributes(std::move(response_headers.get_object_ref()));
#endif
                    }

                    get_output()->begin_string(string_type, contentLength);
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
            curl_slist *curl_headers;
            bool owns_easy, owns_curl;

            core::value response_headers;

            core::istream_handle input_handle;

        public:
            curl_parser(const core::value &url /* URL may include headers as attributes if CPPDATALIB_ENABLE_ATTRIBUTES is defined */,
                        const std::string &verb = "GET",
                        core::istream_handle input = core::istream_handle(),
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
                , curl_headers(nullptr)
                , owns_easy(false)
                , owns_curl(false)
                , input_handle(input)
            {
                reset();
            }
            ~curl_parser()
            {
                if (owns_easy)
                    curl_easy_cleanup(easy);
                else
                    curl_easy_setopt(easy, CURLOPT_HTTPHEADER, NULL);

                if (owns_curl)
                    curl_multi_cleanup(curl);
                if (curl_headers)
                    curl_slist_free_all(curl_headers);
            }

            bool busy() const {return stream_input::busy() || redirects;}

        protected:
            static int seek_callback(void *userdata, curl_off_t offset, int origin)
            {
                if (origin != SEEK_SET)
                    return CURL_SEEKFUNC_FAIL;

                curl_parser *p = static_cast<curl_parser *>(userdata);

                if (!p->input_handle.valid())
                    return CURL_SEEKFUNC_CANTSEEK;

                return p->input_handle.stream().seekg(offset)? CURL_SEEKFUNC_OK: CURL_SEEKFUNC_CANTSEEK;
            }

            static size_t read_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
            {
                curl_parser *p = static_cast<curl_parser *>(userdata);

                if (!p->input_handle.valid())
                    return 0;

                for (size_t i = 0; i < size*nmemb; ++i)
                {
                    core::istream::int_type c = p->input_handle.stream().get();

                    if (c > 0xff)
                        throw core::error("HTTP - invalid encoding used for input to request");
                    else if (c == EOF)
                        return i;

                    ptr[i] = c;
                }

                return size*nmemb;
            }

            static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
            {
                curl_parser *p = static_cast<curl_parser *>(userdata);

                if (p->get_output()->current_container() != core::string) // String not started yet, handle obtaining Content-Length and Content-Type
                {
                    core::value string("", 0, core::blob, true);

                    // Attempt to obtain HTTP response code
                    long responseCode;
                    curl_easy_getinfo(p->easy, CURLINFO_RESPONSE_CODE, &responseCode);

#ifndef CPPDATALIB_ENABLE_ATTRIBUTES
                    if (responseCode >= 400)
                        throw http_error(responseCode, "request failed with error " + std::to_string(responseCode));
#endif

                    p->response_headers.member(core::value("")) = core::value(responseCode);

                    // Attempt to obtain content length
                    core::optional_size contentLength;

                    if (core::ascii_lowercase_copy(p->verb) != "head" &&
                            p->response_headers.is_member("content-length") &&
                            p->response_headers.const_member("transfer-encoding").get_string("identity") == "identity")
                        contentLength = core::optional_size(p->response_headers.const_member("content-length").as_uint());

                    // Attempt to obtain content type to adjust output string's type
                    if (p->response_headers.is_member("content-type"))
                    {
                        std::string contentType = core::ascii_lowercase_copy(p->response_headers.const_member("content-type").as_string());

                        if (contentType.substr(0, 5) == "text/")
                            string.set_subtype(contentType.find("charset=utf-8") != contentType.npos? core::normal: core::clob);
                    }

                    // Attach headers we got for writing to the string if possible
                    // NOTE: this will never include trailer headers, as a limitation of how
                    // attribute information is passed on to the output format
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                    string.set_attributes(p->response_headers.get_object_ref());
#endif

                    p->get_output()->begin_string(string, contentLength);
                }

                p->get_output()->append_to_string(core::value(ptr, size*nmemb, core::normal, true));
                return size*nmemb;
            }

            static size_t header_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
            {
                std::string header(ptr, size*nmemb);
                size_t off = header.find(':');

                if (off != header.npos) // Do we really have a valid header?
                {
                    std::string first = header.substr(0, off);
                    std::string second = header.substr(off+1);

                    curl_parser *p = static_cast<curl_parser *>(userdata);
                    p->response_headers.add_member_at_end(core::value(core::ascii_lowercase_copy(first)), core::value(core::ascii_trim_copy(second)));
                }

                return size*nmemb;
            }

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

                    // Set up URL
                    curl_easy_setopt(easy, CURLOPT_URL, working_url.as_string().c_str());

                    // Set up input reader
                    curl_easy_setopt(easy, CURLOPT_READFUNCTION, read_callback);
                    curl_easy_setopt(easy, CURLOPT_READDATA, this);

                    // Set up seek functions
                    curl_easy_setopt(easy, CURLOPT_SEEKFUNCTION, seek_callback);
                    curl_easy_setopt(easy, CURLOPT_SEEKDATA, this);

                    // Set up output writer
                    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, write_callback);
                    curl_easy_setopt(easy, CURLOPT_WRITEDATA, this);

                    // Set up header writer
                    curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, header_callback);
                    curl_easy_setopt(easy, CURLOPT_HEADERDATA, this);
                    response_headers.set_null();

                    // Set up verb
                    // TODO: setting verb needs work?
                    auto lowerverb = core::ascii_lowercase_copy(verb);
                    if (lowerverb == "get")
                        curl_easy_setopt(easy, CURLOPT_HTTPGET, 1L);
                    else if (lowerverb == "head")
                    {
                        curl_easy_setopt(easy, CURLOPT_HTTPGET, 1L);
                        curl_easy_setopt(easy, CURLOPT_NOBODY, 1L);
                    }
                    else if (lowerverb == "post")
                        curl_easy_setopt(easy, CURLOPT_POST, 1L);
                    else if (lowerverb == "put")
                        curl_easy_setopt(easy, CURLOPT_PUT, 1L);
                    else
                    {
                        curl_easy_setopt(easy, CURLOPT_CUSTOMREQUEST, verb.c_str());
                        curl_easy_setopt(easy, CURLOPT_NOBODY, 0L);
                    }

                    // Set up redirections
                    // TODO: setting up redirects and following them needs work?
                    curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, (long) (maximum_redirects >= 0));
                    curl_easy_setopt(easy, CURLOPT_MAXREDIRS, (long) maximum_redirects);

                    // Set up proxy
                    if (proxy_settings.object_size())
                    {
                        curl_easy_setopt(easy, CURLOPT_PROXYTYPE, working_url.as_string().substr(0, 5) == "https"? CURLPROXY_HTTPS: CURLPROXY_HTTP);
                        curl_easy_setopt(easy, CURLOPT_PROXY, proxy_settings.const_member("host").as_string().c_str());
                        curl_easy_setopt(easy, CURLOPT_PROXYPORT, proxy_settings.const_member("port").as_uint(80));
                        curl_easy_setopt(easy, CURLOPT_PROXYUSERNAME, proxy_settings.const_member("username").as_string().c_str());
                        curl_easy_setopt(easy, CURLOPT_PROXYPASSWORD, proxy_settings.const_member("password").as_string().c_str());
                    }

                    // Set up request headers
                    if (curl_headers)
                    {
                        curl_slist_free_all(curl_headers);
                        curl_headers = nullptr;
                    }

#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                    for (const auto &header: working_url.get_attributes())
                    {
                        curl_headers = curl_slist_append(curl_headers, (header.first.as_string() + ": " + header.second.as_string()).c_str());
                        if (!curl_headers)
                            throw core::error("internal libcurl error");
                    }
#endif

                    for (const auto &header: headers)
                    {
                        curl_headers = curl_slist_append(curl_headers, (header.first.as_string() + ": " + header.second.as_string()).c_str());
                        if (!curl_headers)
                            throw core::error("internal libcurl error");
                    }

                    // Transfer as chunked, since we don't know precisely how long the input is
                    if (input_handle.valid())
                        curl_headers = curl_slist_append(curl_headers, "Transfer-Encoding: chunked");
                    curl_easy_setopt(easy, CURLOPT_HTTPHEADER, curl_headers);

                    // Set keepalive flag for connection
                    curl_easy_setopt(easy, CURLOPT_TCP_KEEPALIVE, 1L);
                    curl_easy_setopt(easy, CURLOPT_TCP_KEEPIDLE, 120L);
                    curl_easy_setopt(easy, CURLOPT_TCP_KEEPINTVL, 60L);

                    working_url.set_string(new_url);
                } catch (const std::exception &e) {
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
                    char err[CURL_ERROR_SIZE];

                    curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, err);

                    CURLcode result = curl_easy_perform(easy);

                    if (result == CURLE_OK)
                    {
                        // End string, since our curl callbacks header_callback and write_callback don't do that
                        get_output()->end_string(core::value("", 0, get_output()->current_container_subtype(), true));
                    }
                    else
                        throw core::custom_error(err);
                } catch (const std::exception &e) {
                    throw core::custom_error("HTTP - " + std::string(e.what()));
                }
            }
        };
#endif

        inline void http_initialize()
        {
#ifdef CPPDATALIB_ENABLE_CURL_NETWORK
            curl_global_init(CURL_GLOBAL_DEFAULT);
#endif
        }

        inline void http_deinitialize()
        {
#ifdef CPPDATALIB_ENABLE_CURL_NETWORK
            curl_global_cleanup();
#endif
        }

        /*
         * parser - generic parsing class for HTTP(S) requests, using various backends.
         *
         * Error messages between backends will differ, but the basic request structure and options should remain the same.
         *
         * To push data (for example, in a POST request) you should do the following:
         *
         *     cppdatalib::http::parser p(args...);
         *     cppdatalib::json::stream_writer writer(p.input_stream()); // Or any stream writer
         *     p.set_input(writer);
         *     p >> std::cout; // Or any output
         *
         * If attributes are disabled:
         *     - This class throws http_error exceptions on HTTP responses >= 400
         *     - The output is a single string or temporary_string with the body of the request, with no headers included
         *     - The exact HTTP response code is not accessible programmatically
         *
         * If attributes are enabled:
         *     - This class does not throw exceptions on HTTP responses >= 400
         *     - The output is a single string or temporary_string with the body of the request,
         *           with headers included as attributes (i.e. header name = attribute key, header value = attribute value)
         *     - The special attribute included in the request with the key "" (i.e. the empty string) contains the HTTP response code
         *
         * The constructor attributes `context` and `s_context` (standing for "secondary context") have different meanings for different backends:
         *
         *     - Qt: `context` is a handle to an existing `QNetworkAccessManager` instance. `s_context` is not used.
         *     - POCO: `context` is a handle to an existing `HTTPClientSession` instance. `s_context` is a handle to an existing `HTTPSClientSession` instance.
         *     - CURL: `context` is a handle to an existing CURL easy instance. `s_context` is a handle to an existing CURL multi instance (not currently used).
         */
        class parser : public core::stream_input
        {
            core::value url;
            core::value headers;
            std::string verb;
            int maximum_redirects;
            core::object_t proxy_settings;
            core::network_library interface_;
            core::stream_input *interface_stream;
            void *context, *s_context;

            core::istream_handle input_handle;

        public:
            parser(const core::value &url /* URL may include headers as attributes if CPPDATALIB_ENABLE_ATTRIBUTES is defined */,
                   core::network_library interface_ = core::default_network_library,
                   const std::string &verb = "GET",
                   core::istream_handle input = core::istream_handle(),
                   const core::object_t &headers = core::object_t(), /* headers must NOT contain a Range field or Transfer-Encoding field */
                   /*
                    * -1 if no limit, 0 disallows redirects of any sort, 1 is single redirect allowed, etc.
                    */
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
                , interface_(core::unknown_network_library)
                , interface_stream(nullptr)
                , context(context)
                , s_context(s_context)
                , input_handle(input)
            {
                set_interface(interface_);
                reset();
            }
            ~parser()
            {
                delete interface_stream;
            }

            void set_interface(core::network_library interface_)
            {
                (void) context;
                (void) s_context;

                if (this->interface_ != interface_)
                {
                    this->interface_ = interface_;
                    delete interface_stream;

                    if (interface_ >= core::network_library_count)
                    {
                        interface_stream = nullptr;
                        return;
                    }

                    switch (interface_)
                    {
                        case core::unknown_network_library:
                        default:
                            interface_stream = nullptr;
                            break;
#ifdef CPPDATALIB_ENABLE_QT_NETWORK
                        case core::qt_network_library:
                            interface_stream = new qt_parser(url, verb, input_handle, headers.get_object_unchecked(), maximum_redirects, proxy_settings,
                                                             static_cast<QNetworkAccessManager *>(context));
                            if (get_output())
                                interface_stream->set_output(*get_output());
                            break;
#endif
#ifdef CPPDATALIB_ENABLE_POCO_NETWORK
                        case core::poco_network_library:
                            interface_stream = new poco_parser(url, verb, input_handle, headers.get_object_unchecked(), maximum_redirects, proxy_settings,
                                                               static_cast<Poco::Net::HTTPClientSession *>(context),
                                                               static_cast<Poco::Net::HTTPSClientSession *>(s_context));
                            if (get_output())
                                interface_stream->set_output(*get_output());
                            break;
#endif
#ifdef CPPDATALIB_ENABLE_CURL_NETWORK
                        case core::curl_network_library:
                            interface_stream = new curl_parser(url, verb, input_handle, headers.get_object_unchecked(), maximum_redirects, proxy_settings,
                                                               static_cast<CURL *>(context),
                                                               static_cast<CURLM *>(s_context));
                            if (get_output())
                                interface_stream->set_output(*get_output());
                            break;
#endif
                    }

                    reset();
                }
            }
            core::network_library get_interface() const {return interface_;}

            int max_redirects() const {return maximum_redirects;}
            void set_max_redirects(int max) {maximum_redirects = max;}

            bool busy() const {return stream_input::busy() || (interface_stream && interface_stream->busy());}

            // `path` is a either a single integer, which specifies where to start reading from,
            // or an array containing two integers, which specify a range to read
            bool seek_to(const core::value &path, bool allow_slow_seeking = false)
            {
                (void) allow_slow_seeking;

                if (path.is_array())
                    headers["Range"] = core::value("bytes " + stdx::to_string(path.element(0).as_uint()) + "-" + (path.array_size() > 1? stdx::to_string(path.element(1).as_uint()): ""));
                else if (path.is_int() || path.is_uint())
                    headers["Range"] = core::value("bytes " + stdx::to_string(path.as_uint()));
                else
                    return false;

                reset();
                return true;
            }

            void cancel_seek()
            {
                headers.erase_member("Range");
                reset();
            }

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
