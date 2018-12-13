/*
 * Plugin Includes
 */
#include "include/common/Logger.hpp"

/*
========================================================================================================
	Operators
========================================================================================================
*/

Logger& operator<<(Logger& logger, const std::string& str) {
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

Logger& operator<<(Logger& logger, const std::string&& str) {
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

Logger& operator<<(Logger& logger, Logger&) {
	return logger;
}