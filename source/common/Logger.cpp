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

QColor Logger::_internal_color = QColor(255, 255, 255);

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

QColor
Logger::colorInfo() {
	return QColor("#ffffff");
}

QColor
Logger::colorError() {
	return QColor("#a11526");
}

QColor
Logger::colorWarning() {
	return QColor("#ff760d");
}

QColor
Logger::colorCustom(unsigned int color) {
	return QColor(color);
}

Logger::LoggerBegin
Logger::begin() {
	return Logger::LoggerBegin();
}

Logger::LoggerEnd
Logger::end() {
	return Logger::LoggerEnd();
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
		try {
			logger.insertHtml(QString("<font color=\"%1\">%2 "
				"(<font color=\"#ffd359\">%3</font>)</font>")
				.arg(logger._internal_color.name(QColor::HexArgb))
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
		try {
			logger.insertHtml(QString("<font color=\"%1\">%2 "
				"(<font color=\"#ffd359\">%3</font>)</font>")
				.arg(logger._internal_color.name(QColor::HexArgb))
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
operator<<(Logger& logger, const QColor& color) {
	logger._internal_color = color;
	return logger;
}

void
operator<<(Logger& logger, Logger::LoggerEnd end) {
	logger.insertHtml(QString::fromStdString(end.end));
	logger.m_mutex.unlock();
}

Logger&
operator<<(Logger& logger, Logger::LoggerBegin begin) {
	logger.m_mutex.lock();
	return logger;
}