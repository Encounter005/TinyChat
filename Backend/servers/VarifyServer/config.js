const fs = require("fs");
let config = JSON.parse(fs.readFileSync("config.json", "utf8"));
let email_user = config.email.user;
let email_pass = config.email.pass;
let redis_host = config.redis.host;
let redis_port = config.redis.port;
let redis_passwd = config.redis.passwd;
// let mysql_host = config.mysql.host;
// let mysql_port = config.mysql.port;
// let mysql_user = config.mysql.user;
// let mysql_passwd = config.mysql.passwd;
let code_prefix = "code_";
module.exports = {
  email_pass,
  email_user,
  // mysql_host,
  // mysql_port,
  // mysql_user,
  // mysql_passwd,
  redis_port,
  redis_host,
  redis_passwd,
  code_prefix,
};
