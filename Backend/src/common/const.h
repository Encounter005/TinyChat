#ifndef CONST_H_
#define CONST_H_

#include <chrono>
#include <functional>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include <map>
#include <assert.h>
#include <json/value.h>
#include <json/json.h>
#include <json/reader.h>
#include "infra/ConfigManager.h"
#include "infra/LogManager.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class ConfigManager;

enum ErrorCodes {
  SUCCESS = 0,
  ERROR_JSON = 1001, // json parse error
  RPC_FAILED = 1002, // rpc request error
  VarifyExpired = 1003, // varify_code expired
  VarifyCodeError = 1004,
  UserExist = 1005,
  PasswdError = 1006,
  EmailNotMatch = 1007,
};


#endif // CONST_H_
