
ifeq (${OS},Windows_NT)
	ECHO=echo -e
	# Which msys compiler is needed ?
	PATH:=/mingw64/bin:${PATH}
	#PATH:=/ucrt64/bin:${PATH}
	#PATH:=/clang64/bin/:${PATH}

	LDFLAGS += -static
	LDLIBS  += -lws2_32
	EXEXT=.exe
	MSBUILD=/c/Program\ Files/Microsoft\ Visual\ Studio/2022/Community/MSBuild/Current/Bin/amd64/MSBuild.exe
	GCC_BUILD_DIR=build/gcc/win
	
else
	ECHO=echo
	OS=$(shell sed -n 's/^ID=//p' /etc/os-release)
	GCC_BUILD_DIR=build/gcc/${OS}
endif

SRCS=main.cpp fs.cpp is_utf8.cpp util.cpp options.cpp
OBJS=$(patsubst %.cpp,${GCC_BUILD_DIR}/%.o,${SRCS})
PREFIX=srt_ed
GCC_TARGET=${GCC_BUILD_DIR}/${PREFIX}${EXEXT}

FLAGS    += -Wall -W -pedantic -Winvalid-pch
FLAGS    += -fPIC -fPIE -fstack-protector-strong
# -O3 option has a really positive impact on the generated binary performance
FLAGS    += -O3 -ffast-math -march=native -g
CXXFLAGS += $(FLAGS) -std=c++23
LDFLAGS  += -flto -fno-fat-lto-objects
LDFLAGS  += -g

.PHONY: FORCE

ifeq (${OS},Windows_NT)
msvc :
	@${MSBUILD} ${PREFIX}.sln -p:Configuration=Release
endif

gcc : ${GCC_TARGET}

${GCC_TARGET} : version.h ${OBJS}
	$(LINK.cpp) ${OBJS} $(LOADLIBES) $(LDLIBS) -o $@

format :
	@echo "Formatting all the .cpp and .h files with clang"
	@clang-format -style="{ BasedOnStyle: Microsoft, ColumnLimit: 256, IndentWidth: 2, TabWidth: 2, UseTab: Never }" --sort-includes -i *.cpp *.h


cfg:
	@echo "OS: ${OS}"
	@type g++
	@echo "PATH: ${PATH}"
	@echo "CXX: ${CXX}"
	@echo "CXXFLAGS: ${CXXFLAGS}"
	@echo "CPPFLAGS: ${CPPFLAGS}"
	@echo "SRCS: ${SRCS}"
	@echo "LDFLAGS: ${LDFLAGS}"
	@echo "GCC_BUILD_DIR: ${GCC_BUILD_DIR}"
	@echo "GCC_TARGET: ${GCC_TARGET}"

clean: 
	rm -f *.o *.d ${PREFIX} ${PREFIX}.exe
	rm -rf ${GCC_BUILD_DIR} build/msvc


ifneq ($(MAKECMDGOALS),clean)
VERSION=$(shell git describe --abbrev=0 --tags 2>/dev/null || echo 'Unknown_version')
COPYRIGHT=(C) D. LALANNE - MIT License
DECORATION=
COMMIT=$(shell git rev-parse --short HEAD 2>/dev/null || echo 'Unknown_commit')
ISO8601= $(shell date +%Y-%m-%dT%H:%M:%SZ)


# Implicit rules

${GCC_BUILD_DIR}/%.o: %.cpp
	@mkdir -p ${GCC_BUILD_DIR}
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

${GCC_BUILD_DIR}/%${EXEXT}: ${GCC_BUILD_DIR}/%.o
	@mkdir -p ${GCC_BUILD_DIR}
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

${GCC_BUILD_DIR}/%${EXEXT}: %.cpp
	@mkdir -p ${GCC_BUILD_DIR}
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@

${GCC_BUILD_DIR}/%.d: %.cpp
	@echo Building header dependencies from $<
	@mkdir -p ${GCC_BUILD_DIR}
	@echo -n "${GCC_BUILD_DIR}/" > $@
	@$(CXX) -MM $< > $@

# Inclusion des fichiers de dépendance .d
ifdef OBJS
# Automatic generation of the app version.h header
version.h : version_check.txt
	@${ECHO} "Building header $@"
	@${ECHO} "#ifndef VERSION_H\n#define VERSION_H\nstruct\n{\n  std::string name, version, copyright, decoration, commit, created_at;\n} app_info = {\"${PREFIX}\", \"${VERSION}\", \"${COPYRIGHT}\", \"${DECORATION}\", \"${COMMIT}\", \"${ISO8601}\"};\n#endif" >$@
	@dos2unix $@

# To silently update version.h when necessary
version_check.txt : FORCE
	@${ECHO} "Version:${VERSION}, copyright:${COPYRIGHT}, decoration:${DECORATION}" >new_$@
	@-( if [ ! -f $@ ]; then cp new_$@ $@; sleep 0.4; fi )
	@-( if diff new_$@ $@ >/dev/null 2>&1; then rm -f new_$@; else mv -f new_$@ $@; fi )

${GCC_BUILD_DIR}/options.d : version.h

-include $(OBJS:.o=.d)
endif
endif

