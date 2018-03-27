#pragma once

#include "GameLogic.h"

class GameManager
{
public:
	GameManager();

	PlayerType StartGame(int uid);
	void RequestGiveUp(PlayerType player);

	void RacketAction(const RacketRequest& req);

	void OnFrameUpdate();

	int GetPlayerUid(PlayerType player) const;
	PlayerType GetPlayerType(int uid) const;


private:
	void GameEnd();

	uint32_t LeftResyncNeeded()
	{
		uint32_t ret = mLeftResyncNeededInputFrame;
		mLeftResyncNeededInputFrame = 0;
		return ret;
	}

	uint32_t RightResyncNeeded()
	{
		uint32_t ret = mRightResyncNeededInputFrame;
		mRightResyncNeededInputFrame = 0;
		return ret;
	}

private:

	GameLogic mGameLogic;

	int mLeftPlayerUid;
	int mRightPlayerUid;

	uint32_t mLeftResyncNeededInputFrame;
	uint32_t mRightResyncNeededInputFrame;

};

extern std::unique_ptr<GameManager> GGameManager;