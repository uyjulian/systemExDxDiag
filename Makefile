
SOURCES += main.cpp

PROJECT_BASENAME = systemExDxDiag

LDLIBS += -lole32 -loleaut32

RC_LEGALCOPYRIGHT ?= Copyright (C) 2021-2021 Julian Uy; See details of license at license.txt, or the source code location.

include external/ncbind/Rules.lib.make
