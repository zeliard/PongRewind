#pragma once
#include "SharedStruct.h"
#include <array>

template <class BASETYPE, uint32_t MAX_REWIND>
class RewindObject
{
public:
	RewindObject()
	{
		Clear();
	}

	void SetObjectForFrame(uint32_t frame, const BASETYPE& obj)
	{
		/// 현재 시각보가 과거에 삽입은 안됨
		if (frame < mHeadFrame)
			return;

		/// 현재 시각이 Rewind를 위해 보관할 수 있는 최대를 넘는 경우
		if (frame >= mHeadFrame + MAX_REWIND)
		{
			mHeadFrame = frame;
			mHeadIndex = 0;

			mHistory.fill(obj);

			return;
		}

		const BASETYPE& prevObj = mHistory[mHeadIndex];

		/// 현재 프레임 직전까지 바로전 최신값 채우기
		while (mHeadFrame < frame)
		{
			++mHeadIndex;
			if (mHeadIndex == MAX_REWIND)
				mHeadIndex = 0;

			mHistory[mHeadIndex] = prevObj;

			++mHeadFrame;
		}

		mHistory[mHeadIndex] = obj;
	}

	BASETYPE GetObjectForFrame(uint32_t frame) const
	{
		/// 현재 가지고 있는것보다 미래의 값을 요구할 때는 그냥 최신값 리턴
		if (frame > mHeadFrame)
			return mHistory[mHeadIndex];

		/// 범위밖 과거의 값을 요구할때는 가장 과거값 리턴
		auto delta = mHeadFrame - frame;
		if (delta >= MAX_REWIND)
		{
			return mHistory[(mHeadIndex + 1) % MAX_REWIND];
		}

		return mHistory[(mHeadIndex + MAX_REWIND - delta) % MAX_REWIND];
	}

	void Clear()
	{
		mHistory.fill(BASETYPE());
		mHeadFrame = 0;
		mHeadIndex = 0;
	}

private:
	uint32_t mHeadFrame; ///< 현재 프레임
	uint32_t mHeadIndex; ///< 배열내 위치

	std::array<BASETYPE, MAX_REWIND> mHistory;

};

class GameLogic
{
public:
	GameLogic();

	void GiveUp(PlayerType player);
	
	bool OnServerUpdate();

	/// return true if the game status changed
	static bool ChangeGameStatus(GameStatus& stat, PlayerType player, float posDiff, bool shoot);

	/// return true when the game ends
	static bool GetUpdatedGameStatus(GameStatus& stat);

	static unsigned __int64 GetGameStatusHash(const GameStatus& stat);
	
	void ResetGameStatus();

	unsigned int GetWorldFrame() const { return mCurrentWorldFrame; }

	void StartPlaying();

	bool IsPlaying() const;


	void SetGameStatus(unsigned int frame, const GameStatus& stat);
	GameStatus GetGameStatus(unsigned int frame) const;
	
	void SetCurrentGameStatus(const GameStatus& stat);
	GameStatus GetCurrentGameStatus() const;
	


private:

	GameStatus mGameStatus;
	
	RewindObject<GameStatus, MAX_WORLD_FRAME_HISTORY> mFrameHistory;
	unsigned int mCurrentWorldFrame;
};