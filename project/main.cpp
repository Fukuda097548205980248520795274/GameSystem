#include "Game.h"

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
	std::unique_ptr<Game> game = std::make_unique<Game>(1280, 720, "LE3A_14_フクダ_ソウワ");
	return game->Run();
}