#ifndef _SYLAR_LOG_H_
#define _SYLAR_LOG_H_
#include <cstdint>
#include <fstream>
#include <list>
#include <memory>
#include <ostream>
#include <string>
#include <stringstream>  //
#include <vector>
namespace sylar {

//日志事件
class LogEvent {
 public:
  typedef std::shared_ptr<LogEvent> ptr;
  LogEvent();

  const char *getFile() const { return m_file; };
  int32_t getLine() const { return m_line; };
  uint32_t getElapse() const { return m_elapse; };
  uint32_t getThreadId() const { return m_threadId; };
  uint32_t getFiberId() const { return m_fiberId; };
  uint64_t getTime() const { return m_time; };
  std::string getContent() const { return m_content; };

 private:
  const char *m_file = nullptr;  //文件名
  int32_t m_line = 0;            //行号
  uint32_t m_elapse;             //程序启动到现在的毫秒数
  uint32_t m_threadId = 0;       //线程id
  uint32_t m_fiberId = 0;        //协程id
  uint64_t m_time;               // 时间戳
  std::string m_content;
};

// 日志级别
class LogLevel {
 public:
  enum Level { UNKNOW = 0, DEBUG = 1, INFO = 2, WARN = 3, ERROR = 4, FATAL = 5 };
  static const char *ToString(LogLevel::Level level);
};

// 日志格式器
class LogFormatter {
 public:
  using ptr = std::shared_ptr<LogFormatter>;
  LogFormatter(const std::string &pattern);

  // %t %thread_id %m%n
  std::string format(LogLevel::Level level, LogEvent::ptr event);

 public:
  class FormatItem {
   public:
    using ptr = std::shared_ptr<FormatItem>;
    virtual ~FormatItem() {}
    virtual void format(std::ostream &os, LogLevel::Level level, LogEvent::ptr event) = 0;
  };
  void init();

 private:
  std::string m_pattern;
  std::vector<FormatItem::ptr> m_items;
};

// 日志输出地
class LogAppender {
 public:
  typedef std::shared_ptr<LogAppender> ptr;
  virtual ~LogAppender() {}

  virtual void log(LogLevel::Level level, LogEvent::ptr event) = 0;

  void setFormatter(LogFormatter::ptr val) { m_formatter = val; }

 protected:
  LogLevel::Level m_level;
  LogFormatter::ptr m_formatter;
};

// 日志器
class Logger {
 public:
  typedef std::shared_ptr<Logger> ptr;

  Logger(const std::string &name = "root");
  void log(LogLevel::Level level, LogEvent::ptr &event);

  void debug(LogEvent::ptr event);
  void info(LogEvent::ptr event);
  void warn(LogEvent::ptr event);
  void error(LogEvent::ptr event);
  void fatal(LogEvent::ptr event);

  void addAppender(LogAppender::ptr appender);
  void delAppender(LogAppender::ptr appender);
  LogLevel::Level getLevel() const { return m_level; }
  void setLevel(LogLevel::Level val) { m_level = val; }

 private:
  std::string m_name;                           // 日志名称
  LogLevel::Level m_level;                      // 日志级别
  std::list<LogAppender::ptr> m_appender_list;  // Appender 集合
};

// 输出到控制台的Appender
class StdoutLogAppender : public LogAppender {
 public:
  typedef std::shared_ptr<StdoutLogAppender> ptr;
  void log(LogLevel::Level, LogEvent::ptr event) override;
};

// 定义输出到文件的Appender
class FileLogAppender : public LogAppender {
  typedef std::shared_ptr<FileLogAppender> ptr;
  FileLogAppender(const std::string &filename);
  void log(LogLevel::Level level, LogEvent::ptr event) override;

  //  重新打开文件，文件打开成功，返回true，失败返回false
  bool reopen();

 private:
  std::string m_filename;
  std::ofstream m_filestream;
};

}  // namespace sylar
#endif  //_SYLAR_LOG_H_