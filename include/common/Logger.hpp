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

		static Logger& instance() {
			static Logger _instance;
			return _instance;
		}
	
	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
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

		Logger() : m_editOutput(nullptr) {};

		Logger(Logger&&) = delete;

		Logger(Logger&) = delete;

		~Logger() = default;

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		void setOutput(QTextEdit* output) {
			m_editOutput = output;
		}

		Logger& color_info() {
			std::lock_guard<std::mutex> lock(m_mutex);
			m_internalColor = QColor("#ffffff");

			return *this;
		}

		Logger& color_error() {
			std::lock_guard<std::mutex> lock(m_mutex);
			m_internalColor = QColor("#a11526");

			return *this;
		}

		Logger& color_warning() {
			std::lock_guard<std::mutex> lock(m_mutex);
			m_internalColor = QColor("#ff760d");

			return *this;
		}

		Logger& color_custom(unsigned int color) {
			std::lock_guard<std::mutex> lock(m_mutex);
			m_internalColor = QColor(color);

			return *this;
		}

	/*
	====================================================================================================
		Operators
	====================================================================================================
	*/
	public:

		friend Logger& operator<<(Logger&, const std::string&);

		friend Logger& operator<<(Logger&, const std::string&&);

		friend Logger& operator<<(Logger&, Logger&);

	private:

		Logger operator=(const Logger&) = delete;

		Logger& operator=(Logger&&) = delete;
};

/*
========================================================================================================
	Functions Declarations
========================================================================================================
*/

Logger& operator<<(Logger& logger, const std::string& str);

Logger& operator<<(Logger& logger, const std::string&& str);

Logger& operator<<(Logger& logger, Logger&);

#define log_info Logger::instance() << Logger::instance().color_info()
#define log_warn Logger::instance() << Logger::instance().color_warning()
#define log_error Logger::instance() << Logger::instance().color_error()
#define log_custom(color) Logger::instance() << Logger::instance().color_custom(color)

#define LOG_STREAMDECK_CLIENT 0xbbf5ff
#define LOG_STREAMDECK 0xcaff9e
#define LOG_STREAMDECK_MANAGER 0x7752ff