#include "player.h"

Player::Player ()
{
	gst_init(nullptr, nullptr);
}

Player::~Player() 
{
	
}

// void Player::playStream(string uri)
// {
// 	execlp("gst-launch-1.0", "-e", "playbin", ("uri=" + uri).c_str(), nullptr);
// }