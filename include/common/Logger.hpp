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
		Static Class Attributes
	====================================================================================================
	*/
	private:

		thread_local static QColor _internal_color;

	/*
	====================================================================================================
		Static Class Functions
	====================================================================================================
	*/
	public:

		static Logger&
		instance();
	
	/*
	================================================================================================
		Instance Data Members
	================================================================================================
	*/
	private:

		std::mutex m_mutex;

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

		Logger&
		colorInfo();

		Logger&
		colorError();

		Logger&
		colorWarning();

		Logger&
		colorCustom(unsigned int color);

	private:

		void insertHtml(const QString& text);

	/*
	====================================================================================================
		Operators
	====================================================================================================
	*/
	public:

		friend Logger&
		operator<<(Logger&, const std::string&);

		friend Logger&
		operator<<(Logger&, const std::string&&);

		friend Logger&
		operator<<(Logger&, Logger&);

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
operator<<(Logger& logger, const std::string& str);

Logger&
operator<<(Logger& logger, const std::string&& str);

Logger&
operator<<(Logger& logger, Logger&);

#define log_info Logger::instance() << Logger::instance().colorInfo()
#define log_warn Logger::instance() << Logger::instance().colorWarning()
#define log_error Logger::instance() << Logger::instance().colorError()
#define log_custom(color) Logger::instance() << Logger::instance().colorCustom(color)