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
	Types Definitions
========================================================================================================
*/

class Logger {

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

		QTextEdit* m_editOutput;

		QColor m_internalColor;

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