#ifndef WEBSOCKETMM_HTTPUTIL_H
#define WEBSOCKETMM_HTTPUTIL_H

#include <websocketmm/beast/beast.h>
beast::string_view mime_type(beast::string_view path);
std::string path_cat(beast::string_view base, beast::string_view path);

#endif // WEBSOCKETMM_HTTPUTIL_H
