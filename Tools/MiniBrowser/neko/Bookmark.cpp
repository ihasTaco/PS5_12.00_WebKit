/*
 * Copyright (C) 2020 Sony Interactive Entertainment Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Bookmark.h"

#include "GetLine.h"

using namespace std;

// A simple text file, where title and URL lines are alternately written.
static const char* s_bookmarkFilePath = "/hostapp/MiniBrowserBookmarks.txt";

void addBookmark(Bookmark&& bookmark)
{
    FILE* fout = fopen(s_bookmarkFilePath, "a");
    if (!fout) {
        fprintf(stderr, "Cannot open %s\n", s_bookmarkFilePath);
        return;
    }

    fprintf(fout, "%s\n", bookmark.title.c_str());
    fprintf(fout, "%s\n", bookmark.url.c_str());

    fclose(fout);
}

void listBookmark(std::function<bool(Bookmark&&)>&& callback)
{
    FILE* fin = fopen(s_bookmarkFilePath, "r");
    if (!fin) {
        fprintf(stderr, "Cannot open %s\n", s_bookmarkFilePath);
        return;
    }

    string title;
    string url;
    while (getLine(fin, title) && getLine(fin, url) && callback({ title, url })) { }

    fclose(fin);
}
