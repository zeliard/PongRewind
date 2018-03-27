#include "stdafx.h"
#include "Log.h"
#include "GameManager.h"
#include "ClientManager.h"

std::unique_ptr<GameManager> GGameManager;

GameManager::GameManager() 
	: mLeftPlayerUid(0), mRightPlayerUid(0), mLeftResyncNeededInputFrame(0), mRightResyncNeededInputFrame(0)
{
}


PlayerType GameManager::StartGame(int uid)
{
	if (mLeftPlayerUid > 0 && mRightPlayerUid > 0)
	{
		GConsoleLog->PrintOut(true, "[ERROR] Game Already Started...\n");
		return PlayerType::PLAYER_NONE;
	}

	if (mLeftPlayerUid == 0 && mRightPlayerUid == 0)
	{
		/// first player
		mLeftPlayerUid = uid;
		return PlayerType::PLAYER_LEFT;
	}
	else
	{
		/// second player
		mRightPlayerUid = uid;
		mGameLogic.StartPlaying();
		return PlayerType::PLAYER_RIGHT;
	}
}

void GameManager::RequestGiveUp(PlayerType player)
{
	mGameLogic.GiveUp(player);

	GameStatusBroadcast packet;
	packet.mGameStatus = mGameLogic.GetCurrentGameStatus();

	GClientManager->BroadcastPacket(&packet);

	GameEnd();
}

void GameManager::GameEnd()
{
	mLeftPlayerUid = mRightPlayerUid = 0;
	mGameLogic.ResetGameStatus();

	mLeftResyncNeededInputFrame = mRightResyncNeededInputFrame = 0;
}

void GameManager::RacketAction(const RacketRequest& req)
{
	if (req.mRecentWorldFrame > mGameLogic.GetWorldFrame())
	{
		///클라 해킹
		GConsoleLog->PrintOut(true, "[CHEAT] Client's WorldFrame Error...\n");
		return;
	}

	/// 여기서 리와인드해서 클라가 보내준 프레임으로 돌려서 판정
	
	GameStatus rewound = mGameLogic.GetGameStatus(req.mRecentWorldFrame);
	if (rewound.mWorldFrame != req.mRecentWorldFrame)
	{
		GConsoleLog->PrintOut(true, "[CHEAT] Client's WorldFrame is different from Rewound Frame Status...\n");
		return;
	}

	bool changed = mGameLogic.ChangeGameStatus(rewound, req.mPlayerType, req.mPosDiff, req.mIsShoot);
	if (changed)
	{
		mGameLogic.GetUpdatedGameStatus(rewound);

		if (mGameLogic.GetGameStatusHash(rewound) != req.mStatusHash)
		{
			/// desync상황 
			GConsoleLog->PrintOut(true, "[DESYNC] Detected Status Difference: RF[%d] CF[%d]\n", rewound.mWorldFrame, mGameLogic.GetWorldFrame());

			/// 다음 방송에서 클라로 전송
			if (req.mPlayerType == PlayerType::PLAYER_LEFT)
			{
				mLeftResyncNeededInputFrame = req.mInputFrame;
			}
			else
			{
				mRightResyncNeededInputFrame = req.mInputFrame;
			}
		}
	}

	// 판정은 리와인드 시점으로, 적용은 현재 시점으로
	GameStatus current = mGameLogic.GetCurrentGameStatus();

	bool apply = mGameLogic.ChangeGameStatus(current, req.mPlayerType, req.mPosDiff, req.mIsShoot);
	if (apply)
	{
		/// 실제로 적용
		mGameLogic.SetCurrentGameStatus(current);
	}
}

void GameManager::OnFrameUpdate()
{
	if (!mGameLogic.IsPlaying())
		return;

	bool gameOver = mGameLogic.OnServerUpdate();
	
	GameStatusBroadcast packet;
	packet.mGameStatus = mGameLogic.GetCurrentGameStatus();
	packet.mLeftResyncNeededInputFrame = LeftResyncNeeded();
	packet.mRightResyncNeededInputFrame = RightResyncNeeded();
	
	//TEST: 패킷 임의로 방송 늦추기 실험
	uint32_t delay = rand() % 200; // ms

	GClientManager->DelayedBroadcastGameStatus(delay, packet);

	if (gameOver)
	{
		GameEnd();
	}
}

int GameManager::GetPlayerUid(PlayerType player) const
{
	if (player == PlayerType::PLAYER_LEFT)
		return mLeftPlayerUid;

	if (player == PlayerType::PLAYER_RIGHT)
		return mRightPlayerUid;

	return 0;
}

PlayerType GameManager::GetPlayerType(int uid) const
{
	if (mLeftPlayerUid == uid)
		return PlayerType::PLAYER_LEFT;

	if (mRightPlayerUid == uid)
		return PlayerType::PLAYER_RIGHT;

	return PlayerType::PLAYER_NONE;
}