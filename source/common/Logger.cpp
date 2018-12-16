/*
 * Plugin Includes
 */
#include "include/common/Logger.hpp"

#define FORCE_DEBUG

/*
========================================================================================================
	Static Class Attributes Initializations
========================================================================================================
*/

thread_local QColor Logger::m_internalColor = QColor(255, 255, 255);

/*
========================================================================================================
	Singleton Handling
========================================================================================================
*/

Logger&
Logger::instance() {
	static Logger _instance;
	return _instance;
}

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

Logger::Logger() :
	m_editOutput(nullptr) {
};

/*
========================================================================================================
	Messages Handling
========================================================================================================
*/

void
Logger::output(QTextEdit* output) {
	if(m_editOutput != nullptr) {
		m_loggerImpl.disconnect(
			&m_loggerImpl,
			&LoggerPrivateImpl::insertHtml,
			m_editOutput,
			&QTextEdit::insertHtml
		);
	}
	m_editOutput = output;
	m_loggerImpl.connect(
		&m_loggerImpl,
		&LoggerPrivateImpl::insertHtml,
		m_editOutput,
		&QTextEdit::insertHtml
	);
}

Logger&
Logger::colorInfo() {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_internalColor = QColor("#ffffff");

	return *this;
}

Logger&
Logger::colorError() {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_internalColor = QColor("#a11526");

	return *this;
}

Logger&
Logger::colorWarning() {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_internalColor = QColor("#ff760d");

	return *this;
}

Logger&
Logger::colorCustom(unsigned int color) {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_internalColor = QColor(color);

	return *this;
}

/*
========================================================================================================
	Text Handling
========================================================================================================
*/

void Logger::insertHtml(const QString& text) {
	emit m_loggerImpl.insertHtml(text);
}

/*
========================================================================================================
	Operators
========================================================================================================
*/

Logger&
operator<<(Logger& logger, const std::string& str) {
#if defined(DEBUG) || defined(FORCE_DEBUG) 
	if(logger.m_editOutput != nullptr) {
		std::lock_guard<std::mutex> lock(logger.m_mutex);
		try {
			logger.insertHtml(QString("<font color=\"%1\">%2 "
				"(<font color=\"#ffd359\">%3</font>)</font><br />")
				.arg(logger.m_internalColor.name(QColor::HexArgb))
				.arg(QString::fromStdString(str))
				.arg((quint64)QThread::currentThreadId()));
		}
		catch(std::exception e) {
			std::cout << e.what() << std::endl;
		}
	}
#endif
	return logger;
}

Logger&
operator<<(Logger& logger, const std::string&& str) {
#if defined(DEBUG) || defined(FORCE_DEBUG) 
	if(logger.m_editOutput != nullptr) {
		std::lock_guard<std::mutex> lock(logger.m_mutex);
		try {
			logger.insertHtml(QString("<font color=\"%1\">%2 "
				"(<font color=\"#ffd359\">%3</font>)</font><br />")
				.arg(logger.m_internalColor.name(QColor::HexArgb))
				.arg(QString::fromStdString(str))
				.arg((quint64)QThread::currentThreadId()));
		}
		catch(std::exception e) {
			std::cout << e.what() << std::endl;
		}
	}
#endif
	return logger;
}

Logger&
operator<<(Logger& logger, Logger&) {
	return logger;
}