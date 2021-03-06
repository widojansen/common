/*  Copyright (C) 2014-2020 FastoGT. All right reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

        * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the
    distribution.
        * Neither the name of FastoGT. nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <common/http/http.h>

#include <common/convert2string.h>  // for ConvertFromString
#include <common/sprintf.h>
#include <common/uri/url.h>
#include <common/utils.h>

namespace common {

std::string ConvertToString(http::http_protocol protocol) {
  if (protocol == http::HP_1_0) {
    return HTTP_1_0_PROTOCOL_NAME;
  } else if (protocol == http::HP_1_1) {
    return HTTP_1_1_PROTOCOL_NAME;
  } else if (protocol == http::HP_2_0) {
    return HTTP_2_0_PROTOCOL_NAME;
  }

  NOTREACHED();
  return "UNKNOWN";
}

bool ConvertFromString(const std::string& from, http::http_protocol* out) {
  if (!out) {
    return false;
  }

  if (from == HTTP_1_0_PROTOCOL_NAME) {
    *out = http::HP_1_0;
    return true;
  } else if (from == HTTP_1_1_PROTOCOL_NAME) {
    *out = http::HP_1_1;
    return true;
  } else if (from == HTTP_2_0_PROTOCOL_NAME) {
    *out = http::HP_2_0;
    return true;
  }

  DNOTREACHED() << "Unknown protocol: " << from;
  return false;
}

namespace http {

HttpHeader::HttpHeader() : key(), value() {}

HttpHeader::HttpHeader(const std::string& key, const std::string& value) : key(key), value(value) {}

bool HttpHeader::IsValid() const {
  return !key.empty();
}

HttpRequest::HttpRequest() : method_(), path_(), protocol_(), headers_(), body_() {}

HttpRequest::HttpRequest(http_method method,
                         const uri::Upath& path,
                         http_protocol protocol,
                         const headers_t& headers,
                         const std::string& body)
    : method_(method), path_(path), protocol_(protocol), headers_(headers), body_(body) {}

http::http_protocol HttpRequest::GetProtocol() const {
  return protocol_;
}

headers_t HttpRequest::GetHeaders() const {
  return headers_;
}

bool HttpRequest::IsValid() const {
  return path_.IsValid();
}

uri::Upath HttpRequest::GetPath() const {
  return path_;
}

void HttpRequest::SetPath(const uri::Upath& path) {
  path_ = path;
}

http::http_method HttpRequest::GetMethod() const {
  return method_;
}

std::string HttpRequest::GetBody() const {
  return body_;
}

bool HttpRequest::FindHeaderByKeyAndChange(const std::string& key, bool case_sensitive, header_t new_value) {
  for (size_t i = 0; i < headers_.size(); ++i) {
    std::string cur_key = headers_[i].key;
    if (EqualsASCII(cur_key, key, case_sensitive)) {
      headers_[i] = new_value;
      return true;
    }
  }

  return false;
}

void HttpRequest::RemoveHeaderByKey(const std::string& key, bool case_sensitive) {
  for (size_t i = 0; i < headers_.size(); ++i) {
    std::string curKey = headers_[i].key;
    if (EqualsASCII(curKey, key, case_sensitive)) {
      headers_.erase(headers_.begin(), headers_.begin() + i);
      return;
    }
  }
}

bool HttpRequest::FindHeaderByKey(const std::string& key, bool case_sensitive, header_t* hdr) const {
  if (!hdr) {
    return false;
  }

  for (size_t i = 0; i < headers_.size(); ++i) {
    std::string curKey = headers_[i].key;
    if (EqualsASCII(curKey, key, case_sensitive)) {
      *hdr = headers_[i];
      return true;
    }
  }

  return false;
}

bool HttpRequest::FindHeaderByValue(const std::string& value, bool case_sensitive, header_t* hdr) const {
  if (!hdr) {
    return false;
  }

  for (size_t i = 0; i < headers_.size(); ++i) {
    std::string curKey = headers_[i].value;
    if (EqualsASCII(curKey, value, case_sensitive)) {
      *hdr = headers_[i];
      return true;
    }
  }

  return false;
}

Optional<HttpRequest> MakeHeadRequest(const uri::Upath& path, http_protocol protocol, const headers_t& headers) {
  if (!path.IsValid()) {
    return Optional<HttpRequest>();
  }
  return http::HttpRequest(http::HM_HEAD, path, protocol, headers, std::string());
}

Optional<HttpRequest> MakeGetRequest(const uri::Upath& path,
                                     http_protocol protocol,
                                     const headers_t& headers,
                                     const std::string& body) {
  if (!path.IsValid()) {
    return Optional<HttpRequest>();
  }
  return http::HttpRequest(http::HM_GET, path, protocol, headers, body);
}

Optional<HttpRequest> MakePostRequest(const uri::Upath& path,
                                      http_protocol protocol,
                                      const headers_t& headers,
                                      const std::string& body) {
  if (!path.IsValid()) {
    return Optional<HttpRequest>();
  }
  return http::HttpRequest(http::HM_POST, path, protocol, headers, body);
}

std::pair<http_status, Error> parse_http_request(const std::string& request, HttpRequest* req_out) {
  if (request.empty() || !req_out) {
    return std::make_pair(HS_FORBIDDEN, make_error_inval());
  }

  typedef std::string::size_type string_size_t;

  const string_size_t len = request.size();

  http_method lmethod = HM_GET;
  uri::Upath lpath;
  http_protocol lprotocol = HP_1_0;
  headers_t lheaders;

  string_size_t pos = 0;
  string_size_t start = 0;
  uint8_t line_count = 0;
  while ((pos = request.find("\r\n", start)) != std::string::npos) {
    std::string line = request.substr(start, pos - start);
    if (line_count == 0) {  // GET //POST //HEAD
      string_size_t space = line.find_first_of(' ');
      if (space != std::string::npos) {
        std::string method = line.substr(0, space);
        if (method == "GET" || method == "HEAD" || method == "POST") {
          if (method == "GET") {
            lmethod = HM_GET;
          } else if (method == "HEAD") {
            lmethod = HM_HEAD;
          } else {
            lmethod = HM_POST;
          }

          std::string protcolAndPath = line.substr(space + 1);
          size_t space_protocol = protcolAndPath.find_first_of("HTTP");
          if (space_protocol != std::string::npos) {
            std::string path = protcolAndPath.substr(0, space_protocol - 1);
            if (protcolAndPath[0] != '/') {  // must start with /
              return std::make_pair(HS_BAD_REQUEST, make_error("Bad filename."));
            }

            std::string protocol_str = protcolAndPath.substr(space_protocol);
            if (!ConvertFromString(protocol_str, &lprotocol)) {
              DNOTREACHED() << "Unknown protocol: " << protocol_str;
            }
            lpath = uri::Upath(path);
          } else {
            return std::make_pair(HS_FORBIDDEN, make_error("Not allowed."));
          }
        } else {
          return std::make_pair(HS_NOT_IMPLEMENTED, make_error("That method is not implemented."));
        }
      } else {
        return std::make_pair(HS_NOT_IMPLEMENTED, make_error("That method is not implemented."));
      }
    } else {
      string_size_t delem = line.find_first_of(':');
      if (delem != std::string::npos) {
        std::string key = line.substr(0, delem);
        std::string value = line.substr(delem + 1);
        std::string trimkey;
        TrimWhitespaceASCII(key, TRIM_ALL, &trimkey);

        std::string trimvalue;
        TrimWhitespaceASCII(value, TRIM_ALL, &trimvalue);
        HttpHeader header(trimkey, trimvalue);
        lheaders.push_back(header);
      }
    }
    line_count++;
    start = pos + 2;
  }

  char* lbody = nullptr;
  if (len != start && line_count != 0) {
    const char* request_str = request.c_str() + start;
    lbody = uri::detail::uri_decode(request_str, strlen(request_str));
  }

  if (line_count == 0) {
    utils::freeifnotnull(lbody);
    return std::make_pair(HS_BAD_REQUEST, make_error("Not found CRLF"));
  }

  *req_out = HttpRequest(lmethod, lpath, lprotocol, lheaders, lbody ? lbody : std::string());
  utils::freeifnotnull(lbody);
  return std::make_pair(HS_OK, Error());
}

HttpResponse::HttpResponse() : protocol_(), status_(), headers_(), body_() {}

HttpResponse::HttpResponse(http_protocol protocol,
                           http_status status,
                           const headers_t& headers,
                           const std::string& body)
    : protocol_(protocol), status_(status), headers_(headers), body_(body) {}

bool HttpResponse::FindHeaderByKey(const std::string& key, bool case_sensitive, header_t* hdr) const {
  if (!hdr) {
    return false;
  }

  for (size_t i = 0; i < headers_.size(); ++i) {
    std::string curKey = headers_[i].key;
    if (EqualsASCII(curKey, key, case_sensitive)) {
      *hdr = headers_[i];
      return true;
    }
  }

  return false;
}

void HttpResponse::SetBody(const std::string& body) {
  body_ = body;
}

bool HttpResponse::IsEmptyBody() const {
  return body_.empty();
}

std::string HttpResponse::GetBody() const {
  return body_;
}

Error parse_http_response(const std::string& response, HttpResponse* res_out, size_t* not_parsed) {
  if (response.empty() || !res_out || !not_parsed) {
    return make_error_inval();
  }

  typedef std::string::size_type string_size_t;

  const string_size_t len = response.size();

  string_size_t pos = 0;
  string_size_t start = 0;
  uint8_t line_count = 0;
  http_protocol lprotocol = HP_1_0;
  headers_t lheaders;
  uint16_t lstatus = 0;
  while ((pos = response.find("\r\n", start)) != std::string::npos) {
    std::string line = response.substr(start, pos - start);
    if (line.empty()) {
      start = pos + 2;
      break;
    }

    if (line_count == 0) {
      if (line.size() < 4) {
        return make_error_inval();
      }

      size_t spaceP = line.find_first_of(" ");
      if (spaceP != std::string::npos) {
        std::string status_str = line.substr(spaceP + 1);
        std::string protocol_str = line.substr(0, spaceP);
        size_t http_sep = protocol_str.find_first_of("/");
        if (http_sep == std::string::npos) {
          return make_error_inval();
        }

        if (!ConvertFromString(protocol_str, &lprotocol)) {
          DNOTREACHED() << "Unknown protocol: " << protocol_str;
        }

        size_t status_sep = status_str.find_first_of(" ");
        if (status_sep == std::string::npos) {
          return make_error_inval();
        }

        std::string status_num = status_str.substr(0, status_sep);
        if (!ConvertFromString(status_num, &lstatus)) {
          return make_error_inval();
        }
      } else {
        return make_error_inval();
      }
    } else {
      string_size_t delem = line.find_first_of(':');
      if (delem != std::string::npos) {
        std::string key = line.substr(0, delem);
        std::string value = line.substr(delem + 1);
        std::string trimkey;
        TrimWhitespaceASCII(key, TRIM_ALL, &trimkey);

        std::string trimvalue;
        TrimWhitespaceASCII(value, TRIM_ALL, &trimvalue);
        HttpHeader header(trimkey, trimvalue);
        lheaders.push_back(header);
      }
    }
    line_count++;
    start = pos + 2;
  }

  *not_parsed = 0;
  HttpResponse lres(lprotocol, static_cast<http_status>(lstatus), lheaders, std::string());
  if (len != start && line_count != 0) {
    const char* response_str = response.c_str() + start;
    http::header_t cont;
    if (lres.FindHeaderByKey("Content-Length", false, &cont)) {
      size_t body_len = 0;
      if (ConvertFromString(cont.value, &body_len)) {
        size_t lnot_parsed = len - start;
        if (lnot_parsed == body_len) {  // full
          lres.SetBody(std::string(response_str, body_len));
        } else {
          *not_parsed = lnot_parsed;
        }
      }
    }
  }

  if (line_count == 0) {
    return make_error("Not found CRLF");
  }

  *res_out = lres;
  return Error();
}

}  // namespace http

std::string ConvertToString(http::http_method method) {
  if (method == http::HM_GET) {
    return "GET";
  } else if (method == http::HM_HEAD) {
    return "HEAD";
  } else {
    return "POST";
  }
}

bool ConvertFromString(const std::string& from, http::http_method* out) {
  if (!out) {
    return false;
  }

  if (from == "GET") {
    *out = http::HM_GET;
    return true;
  } else if (from == "HEAD") {
    *out = http::HM_HEAD;
    return true;
  } else if (from == "POST") {
    *out = http::HM_POST;
    return true;
  }

  return false;
}

std::string ConvertToString(http::http_status status) {
  switch (status) {
    case http::HS_CONTINUE:
      return "Continue";
    case http::HS_SWITCH_PROTOCOL:
      return "Switching Protocols";
    case http::HS_OK:
      return "OK";
    case http::HS_CREATED:
      return "Created";
    case http::HS_ACCEPTED:
      return "Accepted";
    case http::HS_NON_AUTH_INFO:
      return "Non-Authoritative Information";
    case http::HS_NO_CONTENT:
      return "No Content";
    case http::HS_RESET_CONTENT:
      return "Reset Content";
    case http::HS_PARTIAL_CONTENT:
      return "Partial Content";
    case http::HS_MULTIPLE_CHOICES:
      return "Multiple Choices";
    case http::HS_MOVED_PERMANENTLY:
      return "Moved Permanently";
    case http::HS_FOUND:
      return "Found";
    case http::HS_SEE_OTHER:
      return "See Other";
    case http::HS_NOT_MODIFIED:
      return "Not Modified";
    case http::HS_USE_PROXY:
      return "Use Proxy";
    case http::HS_TEMPORARY_REDIRECT:
      return "Temporary Redirect";
    case http::HS_PERMANENT_REDIRECT:
      return "Permanent Redirect";
    case http::HS_BAD_REQUEST:
      return "Bad Request";
    case http::HS_UNAUTHORIZED:
      return "Unauthorized";
    case http::HS_PYMENT_REQUIRED:
      return "Payment Required";
    case http::HS_FORBIDDEN:
      return "Forbidden";
    case http::HS_NOT_FOUND:
      return "Not Found";
    case http::HS_NOT_ALLOWED:
      return "Method Not Allowed";
    case http::HS_NOT_ACCEPTABLE:
      return "Not Acceptable";
    case http::HS_PROXY_AUTH_REQUIRED:
      return "Proxy Authentication Required";
    case http::HS_REQUEST_TIMEOUT:
      return "Request Timeout";
    case http::HS_CONFLICT:
      return "Conflict";
    case http::HS_GONE:
      return "Gone";
    case http::HS_LENGTH_REQUIRED:
      return "Length Required";
    case http::HS_PRECONDITION_FAILED:
      return "Precondition Failed";
    case http::HS_PAYLOAD_TOO_LARGE:
      return "Payload Too Large";
    case http::HS_URI_TOO_LONG:
      return "URI Too Long";
    case http::HS_UNSUPPORTED_MEDIA_TYPE:
      return "Unsupported Media Type";
    case http::HS_REQUESTED_RANGE_NOT_SATISFIABLE:
      return "Requested Range Not Satisfiable";
    case http::HS_EXPECTATION_FAILED:
      return "Expectation Failed";
    case http::HS_MISDIRECTED_REQUEST:
      return "Misdirected Request";
    case http::HS_UPGRADE_REQUIRED:
      return "Upgrade Required";
    case http::HS_PRECONDITION_REQUIRED:
      return "Precondition Required";
    case http::HS_TOO_MANY_REQUESTS:
      return "Too Many Requests";
    case http::HS_REQUEST_HEADER_FIELDS_TOO_LARGE:
      return "Request Header Fields Too Large";
    case http::HS_INTERNAL_ERROR:
      return "Internal Server Error";
    case http::HS_NOT_IMPLEMENTED:
      return "Not Implemented";
    case http::HS_BAD_GATEWAY:
      return "Bad Gateway";
    case http::HS_SERVICE_UNAVAILIBLE:
      return "Service Unavailable";
    case http::HS_GATEWAY_TIMEOUT:
      return "Gateway Timeout";
    case http::HS_HTTP_VERSION_NOT_SUPPORTED:
      return "HTTP Version Not Supported";
    case http::HS_NETWORK_AUTH_REQUIRED:
      return "Network Authentication Required";
    default:
      NOTREACHED();
      return "Unknown";
  }
}

std::string ConvertToString(http::HttpHeader header) {
  if (!header.IsValid()) {
    return std::string();
  }

  return header.key + ":" + header.value;
}

std::string ConvertToString(http::HttpRequest request) {
  uri::Upath upath = request.GetPath();
  http::http_method method = request.GetMethod();

  std::string headerout = MemSPrintf("%s %s %s\r\n", ConvertToString(method), upath.GetPath(),
                                     ConvertToString(request.GetProtocol()));  // "GET /hello.htm HTTP/1.1\r\n"
  http::headers_t headers = request.GetHeaders();
  for (size_t i = 0; i < headers.size(); ++i) {
    http::header_t header = headers[i];
    std::string headerStr = ConvertToString(header);
    if (!headerStr.empty()) {
      headerout += headerStr + "\r\n";
    }
  }
  headerout += "\r\n";

  return headerout;
}

buffer_t ConvertToBytes(http::HttpRequest request) {
  std::string request_str = ConvertToString(request);
  buffer_t res;
  if (!ConvertFromString(request_str, &res)) {
    return buffer_t();
  }
  return res;
}

}  // namespace common
