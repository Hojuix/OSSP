#include <stdio.h>

#include "ui_entry.hpp"

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Tree.H>

#include "../local_music/reader.hpp"

int ui_entry(int argc, char** argv) {
    OSSP_LocalMusic_Reader_Query_t* b = OSSP_LocalMusic_Reader_GetAllArtists();

    // CJK doesn't seem to work with default fonts, use a custom one for now
    Fl::set_fonts();
    Fl::set_font(FL_HELVETICA, "NanumSquareRoundOTF");

    Fl_Double_Window* win = new Fl_Double_Window(1280, 800, "OSSP");

    win->begin();
    {
        Fl_Tree *tree = new Fl_Tree(10, 10, win->w()-20, win->h()-20);
        tree->showroot(0); // Do not show "ROOT" of tree

        for (int i = 0; i < b->count; i++) {
            tree->add(b->GetAllArtists_artists[i].name);
        }


        //tree->close();
    }
    win->end();

    win->resizable(win);
    win->show(argc, argv);

    Fl::run();

    return 0;
}
