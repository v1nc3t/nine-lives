#pragma once

// Middle area between the status bar and the page bar: frame in the theme color, selected page name shown large.
// description is shown smaller under it. name nullptr = no page selected yet (empty frame).
void mainScreenDraw(const char *name, const char *description);
