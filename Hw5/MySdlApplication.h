#ifndef _MYSDLAPPLICATION_H_
#define _MYSDLAPPLICATION_H_

#include <SDL.h>
#include <SDL_image.h>
#include <iostream>

class MySdlApplication
{
    private:
        bool running;
        SDL_Window* display;
        void keyboard(const char * key);
        void mouse(SDL_MouseButtonEvent button);
        void motion(const int x, const int y);

    public:
		MySdlApplication();
        int onExecute();
        bool onInit();
		void onEvent(SDL_Event* Event);
        void keyboard();
        void onLoop();
        void onRender();
        void onCleanup();
};

#endif