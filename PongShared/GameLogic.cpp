
#include "GameLogic.h"
#include <math.h>
#include <functional>

template <class T>
inline void hash_combine(unsigned __int64& s, const T& v)
{
	std::hash<T> h;
	s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

GameLogic::GameLogic()
{
	ResetGameStatus();
}

void GameLogic::ResetGameStatus()
{
	mCurrentWorldFrame = 0;
	mGameStatus.ResetStatus();
	mFrameHistory.Clear();
}

void GameLogic::StartPlaying()
{
	GameStatus gs;
	gs.mCurrentStatus = CurrentGameStatus::CGS_STARTED;
	SetCurrentGameStatus(gs);
}

bool GameLogic::IsPlaying() const
{
	return GetCurrentGameStatus().mCurrentStatus == CurrentGameStatus::CGS_STARTED;
}

unsigned __int64 GameLogic::GetGameStatusHash(const GameStatus& stat)
{
	unsigned __int64 result = 0;

	/// 반드시 일치해야 하는 정보만 해싱한다.

	hash_combine(result, stat.mCurrentStatus);
	hash_combine(result, stat.mWorldFrame);
	//hash_combine(result, stat.mLeftRacketPosY);
	//hash_combine(result, stat.mRightRacketPosY);
	hash_combine(result, stat.mLeftShoot);
	hash_combine(result, stat.mRightShoot);
	hash_combine(result, stat.mLeftScore);
	hash_combine(result, stat.mRightScore);
	
	return result;
}

void GameLogic::GiveUp(PlayerType player)
{
	GameStatus gs = GetCurrentGameStatus();

	if (PlayerType::PLAYER_LEFT == player)
	{
		gs.mCurrentStatus = CurrentGameStatus::CGS_GAME_OVER_RIGHT_WIN;
	}
	else
	{
		gs.mCurrentStatus = CurrentGameStatus::CGS_GAME_OVER_LEFT_WIN;
	}

	SetCurrentGameStatus(gs);
}

bool GameLogic::OnServerUpdate()
{
	GameStatus gs = GetCurrentGameStatus();

	gs.mWorldFrame = ++mCurrentWorldFrame;

	bool res = GetUpdatedGameStatus(gs);

	SetCurrentGameStatus(gs);
	
	return res;
}

bool GameLogic::ChangeGameStatus(GameStatus& stat, PlayerType player, float posDiff, bool shoot)
{
	bool isNewShoot = false;

	if (PlayerType::PLAYER_LEFT == player)
	{
		stat.mLeftRacketPosY += posDiff;
		
		/// 이미 발동중이 아닌 경우만 (이미 발동중이라면 일종의 쿨타임 역할)
		if (shoot && stat.mLeftShoot == 0)
		{
			isNewShoot = true;
			stat.mLeftShoot = SHOOT_FRAME_LENGTH;
		}
	}
	else
	{
		stat.mRightRacketPosY += posDiff;

		if (shoot && stat.mRightShoot == 0)
		{
			isNewShoot = true;
			stat.mRightShoot = SHOOT_FRAME_LENGTH;
		}
	}

	return (posDiff != 0) || isNewShoot;
}

bool GameLogic::GetUpdatedGameStatus(GameStatus& stat)
{

	if (stat.mLeftShoot > 0 && stat.mRightShoot > 0)
	{
		/// 둘다 쏜 경우는 어쨌거나 무효가 되어 상태 변화 없음
		stat.mLeftShoot = 0;
		stat.mRightShoot = 0;
		return false;
	}

	/// 왼쪽만 쏜 경우 오른쪽이 HIT되었는지 판정
	if (stat.mLeftShoot > 0)
	{
		float laserPosY = stat.mLeftRacketPosY + RACKET_HEIGHT / 2.f;
		if ( stat.mRightRacketPosY < laserPosY && laserPosY < stat.mRightRacketPosY + RACKET_HEIGHT )
		{
			stat.mLeftShoot = 0; ///< HIT되었으면 강제 리셋
			++stat.mLeftScore; ///< 득점
		}
		else
		{
			--stat.mLeftShoot; ///< HIT 안되었으면 지속시간 프레임 감소
		}
		

		if (stat.mLeftScore == SCORE_FOR_WIN)
		{
			stat.mCurrentStatus = CurrentGameStatus::CGS_GAME_OVER_LEFT_WIN;
			return true;
		}
	}

	/// 오른쪽만 쏜 경우 왼쪽이 HIT되었는지 판정
	if (stat.mRightShoot > 0)
	{
		float laserPosY = stat.mRightRacketPosY + RACKET_HEIGHT / 2.f;
		if (stat.mLeftRacketPosY < laserPosY && laserPosY < stat.mLeftRacketPosY + RACKET_HEIGHT)
		{
			stat.mRightShoot = 0; ///< HIT되었으면 강제 리셋
			++stat.mRightScore; ///< 득점
		}
		else
		{
			--stat.mRightShoot; ///< HIT 안되었으면 지속시간 프레임 감소
		}


		if (stat.mRightScore == SCORE_FOR_WIN)
		{
			stat.mCurrentStatus = CurrentGameStatus::CGS_GAME_OVER_RIGHT_WIN;
			return true;
		}
	}
	

	/// 여기까지 오면 상태변화 없음 그냥 다음 프레임으로 ㄱㄱ
	return false;
}


void GameLogic::SetGameStatus(unsigned int frame, const GameStatus& stat)
{
	mFrameHistory.SetObjectForFrame(frame, stat);
	mCurrentWorldFrame = frame;
}

GameStatus GameLogic::GetGameStatus(unsigned int frame) const
{
	return mFrameHistory.GetObjectForFrame(frame);
}

void GameLogic::SetCurrentGameStatus(const GameStatus& stat)
{
	SetGameStatus(stat.mWorldFrame, stat);
}

GameStatus GameLogic::GetCurrentGameStatus() const
{
	return mFrameHistory.GetObjectForFrame(mCurrentWorldFrame);
}