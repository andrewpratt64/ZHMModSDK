#pragma once

#include <Windows.h>
#include <vector>
#include <spdlog/spdlog.h>
#include <Glacier/ZString.h>

namespace UI
{
	class Console
	{
	private:
		struct LogLine
		{
			spdlog::level::level_enum Level;
			std::string Text;
		};
		
	public:
		static void Init();
		static void Shutdown();
		static void Draw(bool p_HasFocus);
		static void AddLogLine(spdlog::level::level_enum p_Level, const ZString& p_Text);

	private:
		static std::vector<LogLine>* m_LogLines;
		static bool m_ShouldScroll;
		static SRWLOCK m_Lock;
	};
}
