LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := fruit-reloaded
LOCAL_SRC_FILES := \
	attack.cpp board.cpp book.cpp egbb.cpp engine.cpp eval.cpp fen.cpp hash.cpp list.cpp main.cpp \
	material.cpp move.cpp move_check.cpp move_do.cpp move_evasion.cpp move_gen.cpp move_legal.cpp \
	option.cpp pawn.cpp piece.cpp posix.cpp protocol.cpp pst.cpp pv.cpp random.cpp recog.cpp search.cpp \
	search_full.cpp see.cpp sort.cpp square.cpp tb.cpp test.cpp thread.cpp trans.cpp util.cpp \
	value.cpp vector.cpp

LOCAL_CFLAGS    := -pipe -fno-exceptions -fno-rtti -O2 -fstrict-aliasing

include $(BUILD_EXECUTABLE)
