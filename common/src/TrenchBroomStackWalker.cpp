/*
 Copyright (C) 2016 Eric Wasylishen
 
 This file is part of TrenchBroom.
 
 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef _WIN32
#include "StackWalker.h"
#else
#include <execinfo.h>
#endif
#include "TrenchBroomStackWalker.h"

#include <wx/thread.h>

namespace TrenchBroom {
#ifdef _WIN32
    class TBStackWalker : public StackWalker {
    public:
        StringStream m_string;
        TBStackWalker() : StackWalker() {}
        void clear() {
            m_string.str("");
        }
        String asString() {
            return m_string.str();
        }
    protected:
      virtual void OnOutput(LPCSTR szText) {
          m_string << szText;
      }
    };

    static wxMutex s_stackWalkerMutex;
    static TBStackWalker *s_stackWalker;

	String TrenchBroomStackWalker::getStackTrace() {
        // StackWalker is not threadsafe so acquire a mutex
        wxMutexLocker lock(s_stackWalkerMutex);

        if (s_stackWalker == NULL) {
            // create a shared instance on first use
            s_stackWalker = new TBStackWalker();
        }
        s_stackWalker->clear();
        s_stackWalker->ShowCallstack();
		return s_stackWalker->asString();
	}
#else
    String TrenchBroomStackWalker::getStackTrace() {
        const int MaxDepth = 256;
        void *callstack[MaxDepth];
        const int frames = backtrace(callstack, MaxDepth);

        // copy into a vector
        std::vector<void *> framesVec(callstack, callstack + frames);
        if (framesVec.empty())
            return "";
        
        StringStream ss;
        char **strs = backtrace_symbols(&framesVec.front(), static_cast<int>(framesVec.size()));
        for (size_t i = 0; i < framesVec.size(); i++) {
            ss << strs[i] << std::endl;
        }
        free(strs);
        return ss.str();
    }
#endif
}
