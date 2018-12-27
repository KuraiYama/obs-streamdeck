#pragma once

/*
 * Qt Includes
 */
#include <QTextEdit>
#include <QThread>

/*
 * Std Includes
 */
#include <iostream>
#include <thread>
#include <mutex>

/*
========================================================================================================
	Types Predeclarations
========================================================================================================
*/

class Logger;

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

class LoggerPrivateImpl : public QObject {

	Q_OBJECT

	friend class Logger;

	/*
	====================================================================================================
		Types Definitions
	====================================================================================================
	*/
	signals:

		void insertHtml(const QString& text);

};

class Logger {

	/*
	====================================================================================================
		Types Definitions
	====================================================================================================
	*/
	public:

		typedef struct LoggerBegin {

		} LoggerBegin;

		typedef struct LoggerEnd {
			const std::string end = "<br />";
		} LoggerEnd;

	/*
	====================================================================================================
		Static Class Attributes
	====================================================================================================
	*/
	private:

		static QColor _internal_color;

	/*
	====================================================================================================
		Static Class Functions
	====================================================================================================
	*/
	public:

		static Logger&
		instance();

		static QColor
		colorInfo();

		static QColor
		colorError();

		static QColor
		colorWarning();

		static QColor
		colorCustom(unsigned int color);

		static LoggerBegin
		begin();

		static LoggerEnd
		end();
	
	/*
	================================================================================================
		Instance Data Members
	================================================================================================
	*/
	private:

		std::mutex m_mutex;

		std::unique_lock<std::mutex> m_lock;

		LoggerPrivateImpl m_loggerImpl;

		QTextEdit* m_editOutput;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	private:

		Logger();

		Logger(Logger&&) = delete;

		Logger(Logger&) = delete;

		~Logger() = default;

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		void
		output(QTextEdit* output);

	private:

		void
		insertHtml(const QString& text);

	/*
	====================================================================================================
		Operators
	====================================================================================================
	*/
	public:

		friend Logger&
		operator<<(Logger&, LoggerBegin);

		friend Logger&
		operator<<(Logger&, const std::string&);

		friend Logger&
		operator<<(Logger&, const std::string&&);

		friend Logger&
		operator<<(Logger&, const QColor&);

		friend void
		operator<<(Logger&, LoggerEnd);

	private:

		Logger
		operator=(const Logger&) = delete;

		Logger&
		operator=(Logger&&) = delete;

};

/*
========================================================================================================
	Functions Declarations
========================================================================================================
*/

Logger&
operator<<(Logger& logger, Logger::LoggerBegin);

Logger&
operator<<(Logger& logger, const std::string& str);

Logger&
operator<<(Logger& logger, const std::string&& str);

Logger&
operator<<(Logger&, const QColor&);

void
operator<<(Logger& logger, Logger::LoggerEnd);

#define log_info Logger::instance() << Logger::begin() << Logger::colorInfo()
#define log_warn Logger::instance() << Logger::begin() << Logger::colorWarning()
#define log_error Logger::instance() << Logger::begin() << Logger::colorError()
#define log_custom(color) Logger::instance() << Logger::begin() << Logger::colorCustom(color)
#define log_end Logger::end();