#include "stdafx.h"
#include "GUIController.h"
#include "NetController.h"


const int BALL_SIZE = 8;

std::unique_ptr<GUIController> GGuiController;


static void OnMenuEvent(int menuId)
{
	GGuiController->OnMenuEvent(menuId);
}

static void OnEventLoop()
{
	GGuiController->OnRender();
	GNetController->NetworkProcess();
}

static void OnUpdate(int val)
{
	GGuiController->OnUpdate();
}



GUIController::GUIController()
	: mMyPlayerType(PlayerType::PLAYER_NONE)
{
	memset(mTextLine, 0, sizeof(mTextLine));
}

void GUIController::DrawRect(float x, float y, float width, float height)
{
	glBegin(GL_QUADS);
	glVertex2f(x, y);
	glVertex2f(x + width, y);
	glVertex2f(x + width, y + height);
	glVertex2f(x, y + height);
	glEnd();
}

void GUIController::DrawLine(float x1, float y1, float x2, float y2)
{
	glBegin(GL_LINES);
	glVertex2f(x1, y1);
	glVertex2f(x2, y2);
	glEnd();
}


void GUIController::Initialize(int* argcp, char** argv)
{
	glutInit(argcp, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(WORLD_W, WORLD_H);
	glutInitWindowPosition(100, 50);
	glutCreateWindow("Pong Client");


	glutDisplayFunc(::OnEventLoop);
	glutTimerFunc(UPDATE_INTERVAL, ::OnUpdate, 0);


	glViewport(0, 0, WORLD_W, WORLD_H);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, WORLD_W, 0.0f, WORLD_H, 0.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glColor3f(1.0f, 1.0f, 1.0f);

	glutCreateMenu(::OnMenuEvent);
	glutAddMenuEntry("GIVEUP", 1);
	glutAddMenuEntry("START", 2);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	SetTextMessage("RIGHT MOUSE CLICK and START!");
}

void GUIController::OnMenuEvent(int menuId)
{

	if (1 == menuId)
	{
		if (mGameStatus.mCurrentStatus == CurrentGameStatus::CGS_STARTED)
		{
			GNetController->RequestGiveUp();
		}
	}
	else if (2 == menuId)
	{
		if (mGameStatus.mCurrentStatus != CurrentGameStatus::CGS_STARTED)
		{
			GNetController->RequestGameStart();
		}
	}


	glutPostRedisplay();
}

#define VK_W 0x57
#define VK_S 0x53
#define VK_D 0x44

void GUIController::GetRacketInput(float& posDiff, bool& isFire)
{
	if (mMyPlayerType == PlayerType::PLAYER_LEFT)
	{
		if (GetAsyncKeyState(VK_W))
			posDiff = RACKET_SPEED;
		else if (GetAsyncKeyState(VK_S))
			posDiff = -RACKET_SPEED;

		if (GetAsyncKeyState(VK_D))
			isFire = true;
	}
	else
	{
		if (GetAsyncKeyState(VK_UP))
			posDiff = RACKET_SPEED;
		else if (GetAsyncKeyState(VK_DOWN))
			posDiff = -RACKET_SPEED;

		if (GetAsyncKeyState(VK_LEFT))
			isFire = true;
	}

}


void GUIController::OnUpdate()
{
	/// Call OnUpdate again in 'interval' ms
	glutTimerFunc(UPDATE_INTERVAL, ::OnUpdate, 0);

	if (mGameStatus.mCurrentStatus != CurrentGameStatus::CGS_STARTED)
		return;


	float posDiff = 0;
	bool isFire = false;
	GetRacketInput(posDiff, isFire);

	/// 입력 반영
	bool changed = GameLogic::ChangeGameStatus(mGameStatus, mMyPlayerType, posDiff, isFire);

	/// 클라에 상태 선반영
	GameLogic::GetUpdatedGameStatus(mGameStatus);

	
	/// Send Input
	if (changed)
	{
		auto hashval = GameLogic::GetGameStatusHash(mGameStatus);
		GNetController->RequestRacket(posDiff, isFire, mRecentWorldFrame, mRecentInputFrame, hashval);

		mInputHistory[mRecentInputFrame] = InputElem(mRecentInputFrame, posDiff, isFire);
		++mRecentInputFrame;
	}

	/// Redisplay frame
	glutPostRedisplay();

}

void GUIController::OnRender()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	const GameStatus& stat = mGameStatus;

	DrawRect(RACKET_LEFT_X, stat.mLeftRacketPosY, RACKET_WIDTH, RACKET_HEIGHT);
	DrawRect(RACKET_RIGHT_X, stat.mRightRacketPosY, RACKET_WIDTH, RACKET_HEIGHT);

	if (stat.mLeftShoot > 0)
	{
		DrawLine(RACKET_LEFT_X + RACKET_WIDTH, (stat.mLeftRacketPosY + RACKET_HEIGHT / 2.f) , RACKET_RIGHT_X, (stat.mLeftRacketPosY + RACKET_HEIGHT / 2.f));
	}

	if (stat.mRightShoot > 0)
	{
		DrawLine(RACKET_LEFT_X + RACKET_WIDTH, (stat.mRightRacketPosY + RACKET_HEIGHT / 2.f) , RACKET_RIGHT_X, (stat.mRightRacketPosY + RACKET_HEIGHT / 2.f));
	}

	glRasterPos2f(5, WORLD_H - 15);
	
	sprintf_s(mTextLine, "WF[%d] MSG[%s] SCORE[%d:%d]", stat.mWorldFrame, mTextMessage.c_str(), stat.mLeftScore, stat.mRightScore);

	for (int i = 0; i < strlen(mTextLine); ++i)
	{
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, mTextLine[i]);
	}
	
	glutSwapBuffers();
}

void GUIController::Reset()
{
	mMyPlayerType = PlayerType::PLAYER_NONE;

	mGameStatus.ResetStatus();
	mRecentWorldFrame = 0;

	mInputHistory.clear();
	mRecentInputFrame = 0;
}

void GUIController::DoEventLoop()
{
	glutMainLoop();
}


void GUIController::OnGameStart(PlayerType myType)
{
	Reset();

	mMyPlayerType = myType;

	if ( myType == PlayerType::PLAYER_LEFT )
		SetTextMessage("You are Left");
	else
		SetTextMessage("You are Right");

}

void GUIController::OnStatusChange(const GameStatusBroadcast& gs)
{
	const GameStatus& stat = gs.mGameStatus;

	if (mGameStatus.mCurrentStatus != CurrentGameStatus::CGS_STARTED && stat.mCurrentStatus == CurrentGameStatus::CGS_STARTED)
	{
		/// game just started
		mGameStatus.mCurrentStatus = CurrentGameStatus::CGS_STARTED;
	}

	if (mGameStatus.mCurrentStatus == CurrentGameStatus::CGS_STARTED)
	{
		mGameStatus = stat;
		mRecentWorldFrame = stat.mWorldFrame;
	
		if (stat.mCurrentStatus == CurrentGameStatus::CGS_GAME_OVER_LEFT_WIN)
		{
			SetTextMessage("Left Win");
			
			Reset();
		}
		else if (stat.mCurrentStatus == CurrentGameStatus::CGS_GAME_OVER_RIGHT_WIN)
		{
			SetTextMessage("Right Win");
			
			Reset();
		}
		else
		{
			if (gs.mLeftResyncNeededInputFrame > 0 && mMyPlayerType == PlayerType::PLAYER_LEFT)
			{
				Resync(gs.mLeftResyncNeededInputFrame);
			}
			else if (gs.mRightResyncNeededInputFrame > 0 && mMyPlayerType == PlayerType::PLAYER_RIGHT)
			{
				Resync(gs.mRightResyncNeededInputFrame);
			}
		}
	}

}

void GUIController::Resync(uint32_t fromFrame)
{
	/// Input History에서 동기화된 부분 [,fromFrame] 범위까지만 모두 삭제하고 나머지 재적용
	auto it = mInputHistory.begin();

	while (it != mInputHistory.end())
	{
		if (it->second.mInputFrame <= fromFrame)
		{
			it = mInputHistory.erase(it);
		}
		else
		{
			++it;
		}
	}

	/// 최근 IF 이후 모두 반영 동기화
	for (const auto& it : mInputHistory)
	{
		const InputElem& input = it.second;

		std::cout << "RESYNC InputFrame: " << it.first << std::endl;

		/// 최근 입력까지 re-apply
		GameLogic::ChangeGameStatus(mGameStatus, mMyPlayerType, input.mPosDiff, input.mShoot);

		GameLogic::GetUpdatedGameStatus(mGameStatus);

	}

	mInputHistory.clear();
}