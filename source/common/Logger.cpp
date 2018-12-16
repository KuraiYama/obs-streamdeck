/*
 * Plugin Includes
 */
#include "include/common/Logger.hpp"

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
	m_editOutput = output;
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
	Operators
========================================================================================================
*/

Logger&
operator<<(Logger& logger, const std::string& str) {
#ifdef DEBUG
	if(logger.m_editOutput != nullptr) {
		std::lock_guard<std::mutex> lock(logger.m_mutex);
		try {
			logger.m_editOutput->insertHtml(QString("<font color=\"%1\">%2 "
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
#ifdef DEBUG
	if(logger.m_editOutput != nullptr) {
		std::lock_guard<std::mutex> lock(logger.m_mutex);
		try {
			logger.m_editOutput->insertHtml(QString("<font color=\"%1\">%2 "
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