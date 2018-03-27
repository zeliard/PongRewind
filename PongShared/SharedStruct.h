#pragma once


const int WORLD_W = 500;
const int WORLD_H = 200;
const int UPDATE_INTERVAL = 1000 / 20; ///< 20 FPS

const int RACKET_WIDTH = 10;
const int RACKET_HEIGHT = 30;
const int RACKET_SPEED = 5;

const float RACKET_LEFT_X = 10.f;
const float RACKET_RIGHT_X = WORLD_W - RACKET_WIDTH - 10.f;

const int SHOOT_FRAME_LENGTH = 3;

const int SCORE_FOR_WIN = 500;

const int MAX_WORLD_FRAME_HISTORY = 512;

enum class PlayerType : unsigned char
{
	PLAYER_NONE = 0,
	PLAYER_LEFT = 1,
	PLAYER_RIGHT = 2
};

enum class CurrentGameStatus : unsigned char
{
	CGS_NOT_STARTED,
	CGS_STARTED,
	CGS_GAME_OVER_LEFT_WIN,
	CGS_GAME_OVER_RIGHT_WIN
};

enum PacketTypes
{
	PKT_NONE = 0,

	PKT_CS_START = 1,
	PKT_SC_START = 2,

	PKT_CS_RACKET = 21,
	PKT_SC_GAME_STATUS = 22,

	PKT_CS_EXIT = 31,

	PKT_MAX = 1024
};


struct GameStatus
{
	GameStatus()
	{
		ResetStatus();
	}

	void ResetStatus()
	{
		mCurrentStatus = CurrentGameStatus::CGS_NOT_STARTED;
		mWorldFrame = 0;

		mLeftRacketPosY = WORLD_H / 2.f - RACKET_HEIGHT / 2.f;
		mRightRacketPosY = WORLD_H / 2.f - RACKET_HEIGHT / 2.f;

		mLeftShoot = 0;
		mRightShoot = 0;

		mLeftScore = 0;
		mRightScore = 0;
	}

	CurrentGameStatus mCurrentStatus;

	unsigned int mWorldFrame;

	float mLeftRacketPosY;
	float mRightRacketPosY;

	int mLeftShoot; ///< 0보다 크면 쏜거 (몇 프레임동안 유지할지)
	int mRightShoot;

	int mLeftScore;
	int mRightScore;

};


#pragma pack(push, 1)

struct PacketHeader
{
	PacketHeader() : mSize(0), mType(PKT_NONE) {}
	short mSize;
	short mType;
};

struct StartRequest : public PacketHeader
{
	StartRequest()
	{
		mSize = sizeof(StartRequest);
		mType = PKT_CS_START;
	}
};

struct StartResult : public PacketHeader
{
	StartResult()
	{
		mSize = sizeof(StartResult);
		mType = PKT_SC_START;
		mYourPlayerType = PlayerType::PLAYER_NONE;
	}

	PlayerType mYourPlayerType;
};



struct RacketRequest : public PacketHeader
{
	RacketRequest()
	{
		mSize = sizeof(RacketRequest);
		mType = PKT_CS_RACKET;
		mPlayerType = PlayerType::PLAYER_NONE;
		mRecentWorldFrame = 0;
		mInputFrame = 0;
		mPosDiff = 0;
		mIsShoot = false;
		mStatusHash = 0;
	}

	PlayerType mPlayerType;
	unsigned int mRecentWorldFrame;
	unsigned int mInputFrame;

	float mPosDiff;
	bool mIsShoot;
	
	unsigned __int64 mStatusHash;

};


struct GameStatusBroadcast : public PacketHeader
{
	GameStatusBroadcast()
	{
		mSize = sizeof(GameStatusBroadcast);
		mType = PKT_SC_GAME_STATUS;

		mLeftResyncNeededInputFrame = 0;
		mRightResyncNeededInputFrame = 0;
	}

	unsigned int mLeftResyncNeededInputFrame;
	unsigned int mRightResyncNeededInputFrame;

	GameStatus mGameStatus;
};


struct ExitRequest : public PacketHeader
{
	ExitRequest()
	{
		mSize = sizeof(ExitRequest);
		mType = PKT_CS_EXIT;
		mPlayerType = PlayerType::PLAYER_NONE;
	}

	PlayerType mPlayerType;
};



#pragma pack(pop)