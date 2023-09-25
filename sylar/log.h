#ifndef _SYLAR_LOG_H_
#define _SYLAR_LOG_H_
#include <cstdint>
#include <fstream>
#include <list>
#include <map>
#include <memory>
#include <ostream>
#include <sstream>  //
#include <string>
#include <vector>
namespace sylar {

class Logger;

// 日志级别
class LogLevel {
 public:
  enum Level { UNKNOW = 0, DEBUG = 1, INFO = 2, WARN = 3, ERROR = 4, FATAL = 5 };
  /**
   * @brief 将日志级别转成文本输出
   * @param[in] level 日志级别
   */
  static const char *ToString(LogLevel::Level level);
  /**
   * @brief 将文本转换成日志级别
   * @param[in] str 日志级别文本
   */
  static LogLevel::Level FromString(const std::string &str);
};

//日志事件
class LogEvent {
 public:
  typedef std::shared_ptr<LogEvent> ptr;
  /**
   * @brief 构造函数
   * @param[in] logger 日志器
   * @param[in] level 日志级别
   * @param[in] file 文件名
   * @param[in] line 文件行号
   * @param[in] elapse 程序启动依赖的耗时(毫秒)
   * @param[in] thread_id 线程id
   * @param[in] fiber_id 协程id
   * @param[in] time 日志事件(秒)
   * @param[in] thread_name 线程名称
   */
  LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file, int32_t line, uint32_t elapse,
           uint32_t thread_id, uint32_t fiber_id, uint64_t time, const std::string &thread_name);

  const char *getFile() const { return m_file; }
  int32_t getLine() const { return m_line; }
  uint32_t getElapse() const { return m_elapse; }
  uint32_t getThreadId() const { return m_threadId; }
  uint32_t getFiberId() const { return m_fiberId; }
  uint64_t getTime() const { return m_time; }

  /**
   * @brief 返回线程名称
   */
  const std::string &getThreadName() const { return m_threadName; }
  /**
   * @brief 返回日志内容
   */
  std::string getContent() const { return m_ss.str(); }

  std::shared_ptr<Logger> getLogger() const { return m_logger; }
  std::stringstream &getSS() { return m_ss; }  // 必须 & ？？

  /**
   * @brief 格式化写入日志内容
   */
  void format(const char *fmt, ...);

  /**
   * @brief 格式化写入日志内容
   */
  void format(const char *fmt, va_list al);

 private:
  const char *m_file = nullptr;  //文件名
  int32_t m_line = 0;            //行号
  uint32_t m_elapse = 0;         //程序启动到现在的毫秒数
  uint32_t m_threadId = 0;       //线程id
  uint32_t m_fiberId = 0;        //协程id
  uint64_t m_time = 0;           // 时间戳

  std::string m_threadName;
  std::stringstream m_ss;
  std::shared_ptr<Logger> m_logger;
  /// 日志等级
  LogLevel::Level m_level;
};

/**
 * @brief 日志事件包装器
 */
class LogEventWrap {
 public:
  LogEventWrap(LogEvent::ptr e);
  ~LogEventWrap();

  LogEvent::ptr getEvent() const { return m_event; }

  std::stringstream &getSS();

 private:
  LogEvent::ptr m_event;
};

// 日志格式化
class LogFormatter {
 public:
  using ptr = std::shared_ptr<LogFormatter>;
  /**
   * @brief 构造函数
   * @param[in] pattern 格式模板
   * @details
   *  %m 消息
   *  %p 日志级别
   *  %r 累计毫秒数
   *  %c 日志名称
   *  %t 线程id
   *  %n 换行
   *  %d 时间
   *  %f 文件名
   *  %l 行号
   *  %T 制表符
   *  %F 协程id
   *  %N 线程名称
   *
   *  默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
   */
  LogFormatter(const std::string &pattern);

  /**
   * @brief 返回格式化日志文本
   * @param[in] logger 日志器
   * @param[in] level 日志级别
   * @param[in] event 日志事件
   */
  std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
  std::ostream &format(std::ostream &ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

 public:
  /**
   * @brief 日志内容项格式化
   */
  class FormatItem {
   public:
    using ptr = std::shared_ptr<FormatItem>;

    /**
     * @brief 析构函数
     */
    virtual ~FormatItem() {}
    /**
     * @brief 格式化日志到流
     * @param[in, out] os 日志输出流
     * @param[in] logger 日志器
     * @param[in] level 日志等级
     * @param[in] event 日志事件
     */
    virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level,
                        LogEvent::ptr event) = 0;
  };
  void init();

  bool isError() const { return m_error; }
  const std::string getPattern() const { return m_pattern; }

 private:
  /// 日志格式模板
  std::string m_pattern;
  /// 日志格式解析后格式
  std::vector<FormatItem::ptr> m_items;
  /// 是否有错误
  bool m_error = false;
};
/**
 * @brief 日志输出目标
 */
class LogAppender {
  friend class Logger;

 public:
  typedef std::shared_ptr<LogAppender> ptr;
  virtual ~LogAppender() {}
  /**
   * @brief 写入日志
   * @param[in] logger 日志器
   * @param[in] level 日志级别
   * @param[in] event 日志事件
   */
  virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

  virtual std::string toYamlString() = 0;

  void setFormatter(LogFormatter::ptr val);

  LogFormatter::ptr getFormatter();

  LogLevel::Level getLevel() const { return m_level; }

  void setLevel(LogLevel::Level val) { m_level = val; }

 protected:
  /// 日志级别
  LogLevel::Level m_level = LogLevel::DEBUG;
  /// 是否有自己的日志格式器
  bool m_hasFormatter = false;
  LogFormatter::ptr m_formatter;
};

// 日志器
class Logger : public std::enable_shared_from_this<Logger> {
 public:
  typedef std::shared_ptr<Logger> ptr;

  /**
   * @brief 构造函数
   * @param[in] name 日志器名称
   */
  Logger(const std::string &name = "root");

  /**
   * @brief 写日志
   * @param[in] level 日志级别
   * @param[in] event 日志事件
   */
  void log(LogLevel::Level level, LogEvent::ptr &event);

  void debug(LogEvent::ptr event);
  void info(LogEvent::ptr event);
  void warn(LogEvent::ptr event);
  void error(LogEvent::ptr event);
  void fatal(LogEvent::ptr event);

  void addAppender(LogAppender::ptr appender);
  void delAppender(LogAppender::ptr appender);
  void clearAppenders();
  LogLevel::Level getLevel() const { return m_level; }
  void setLevel(LogLevel::Level val) { m_level = val; }

  const std::string &getName() const { return m_name; }
  /**
   * @brief 设置日志格式器
   */
  void setFormatter(LogFormatter::ptr val);

  /**
   * @brief 设置日志格式模板
   */
  void setFormatter(const std::string &val);

  /**
   * @brief 获取日志格式器
   */
  LogFormatter::ptr getFormatter();

  /**
   * @brief 将日志器的配置转成YAML String
   */
  std::string toYamlString();

 private:
  std::string m_name;                       // 日志名称
  LogLevel::Level m_level;                  // 日志级别
  std::list<LogAppender::ptr> m_appenders;  // Appender 集合
  LogFormatter::ptr m_formatter;
  Logger::ptr m_root;
};
/**
 * @brief 输出到控制台的Appender
 */
class StdoutLogAppender : public LogAppender {
 public:
  typedef std::shared_ptr<StdoutLogAppender> ptr;
  void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
};

/**
 * @brief 输出到文件的Appender
 */
class FileLogAppender : public LogAppender {
  typedef std::shared_ptr<FileLogAppender> ptr;
  FileLogAppender(const std::string &filename);
  void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
  std::string toYamlString() override;
  /**
   * @brief 重新打开日志文件
   * @return 成功返回true
   */
  bool reopen();

 private:
  std::string m_filename;
  std::ofstream m_filestream;
  /// 上次重新打开时间
  uint64_t m_lastTime = 0;
};

class LoggerManager {
 public:
  LoggerManager();

  Logger::ptr getLogger(const std::string &name);

  void init();

  Logger::ptr getRoot() const { return m_root; }

  /**
   * @brief 将所有的日志器配置转成YAML String
   */
  std::string toYamlString();

 private:
  /// 日志器容器
  std::map<std::string, Logger::ptr> m_loggers;
  /// 主日志器
  Logger::ptr m_root;
};
/// 日志器管理类单例模式
typedef sylar::Singleton<LoggerManager> LoggerMgr;

}  // namespace sylar
#endif  //_SYLAR_LOG_H_