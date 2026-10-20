#ifndef __LOGGER_H_
#define __LOGGER_H_

#include <string>

using namespace std;

// FILE: append-only text-file logger singleton (runlog.txt by default).
// FITS IN: every module logs errors/warnings through Logger::Instance() -
// there's no exception-based error path anywhere in core/iso/engine.
// WATCH OUT: ~Logger() deletes s_pInstance from inside itself - harmless
// only because this singleton is never explicitly destroyed (it leaks for
// the life of the process); don't call `delete Logger::Instance()` by hand.
class Logger
{
private:
	Logger(){}; // Singleton private constructor
	static Logger * s_pInstance;

	string directory;
    int loggerDirSet = 0;

public:
	~Logger(){ delete s_pInstance;};
	static Logger* Instance();
	void setLoggerDirectory(string directory);
    void deleteLog(string filename);
    void writeToLog(string message, string filename);
};

#endif // __LOGGER_H_
