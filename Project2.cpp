#include "pch.h"
#include <stdio.h>12
#define _USE_INIT_WINDOW_
#define _CRT_SECURE_NO_WARNINGS
#include "tipsware.h"
#include <windows.h>
#include <cstdlib>
#include "Resource.h"

// ===================================================
// 윈도우 초기화
// ===================================================
void InitWindow()
{
    gp_window_title = "chess";
    g_wnd_style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
}

// ===================================================
// 전역 변수
// ===================================================
enum Scene { SCENE_TITLE, SCENE_DIFFICULTY, SCENE_GAME, SCENE_SETTINGS, SCENE_LOBBY, SCENE_WAITING, SCENE_JOIN };
enum Difficulty { DIF_EASY, DIF_NORMAL, DIF_HARD, DIF_HARDCORE, DIF_PVP, DIF_ONLINE_PVP };

Scene      g_scene = SCENE_TITLE;
Difficulty g_difficulty = DIF_EASY;

// 버튼 영역
RECT g_btnStart = { 820, 650, 1100, 720 };
RECT g_btnExit = { 820, 740, 1100, 810 };
RECT g_btnSettings = { 1760,  20, 1900,  70 };
RECT g_btnSetBack = { 820, 600, 1100, 670 };
RECT g_btnSetReselect = { 820, 690, 1100, 760 };
RECT g_btnEasy = { 720,  510, 1000, 580 };  // 쉬움
RECT g_btnNormal = { 720,  600, 1000, 670 };  // 보통
RECT g_btnLocalPVP = { 720,  690, 1000, 760 };  // 오프라인대전
RECT g_btnHard = { 1020, 510, 1300, 580 };  // 어려움
RECT g_btnHardcore = { 1020, 600, 1300, 670 };  // 매우 어려움
RECT g_btnOnlinePVP = { 1020, 690, 1300, 760 };  // 온라인대전
RECT g_btnBack = { 820,  800, 1100, 870 };  // 뒤로가기
RECT g_btnSetRestart = { 820,  780, 1100, 850 };
// 로비 버튼
RECT g_btnHost = { 820, 500, 1100, 570 };
RECT g_btnJoin = { 820, 600, 1100, 670 };
RECT g_btnLobbyBack = { 820, 700, 1100, 770 };

// 턴
bool g_whiteTurn = true;

bool g_whiteInCheck = false;
bool g_blackInCheck = false;
bool g_whiteInCheckmate = false;
bool g_blackInCheckmate = false;
bool g_stalemate = false;

bool g_gameOver = false;
int  g_winner = -1;  // 0=black, 1=white, 2=stalemate

RECT g_resetBtn = { 850, 650, 1100, 700 };

bool  g_whiteAiPlayedThisTurn = false;
DWORD g_aiDelay = 0;

// 프로모션
bool        g_promotionActive = false;
int         g_promoPawnIdx = -1;
bool        g_promoWhite = false;
RECT        g_promoBtns[4];
const char* g_promoText[4] = { "Q", "R", "N", "B" };

bool g_showWhiteThink = false;
bool g_prevEscDown = false;

// 앙파상
int g_enPassantX = -1;
int g_enPassantY = -1;

// 국면 저장
char g_positionHistory[200][32 * 5];
int  g_positionCount = 0;

// RNG
unsigned int g_rng = 1;

// 네트워크
bool  g_isHost = false;
char  g_ipBuffer[16] = "";
void* g_editIP = NULL;
bool  g_isConnected = false;
void* g_serverSocket = NULL;
void* g_clientSocket = NULL;
RECT  g_btnWaitingBack = { 820, 650, 1100, 720 };
RECT  g_btnConnect = { 820, 580, 1100, 650 };
RECT  g_btnJoinBack = { 820, 670, 1100, 740 };

// ✅ 뒤집기
bool g_boardFlipped = false;

// ===================================================
// 체스말
// ===================================================
enum PieceType
{
    pawn, bishop, king, knight, queen, rook,       // 검은(0~5)
    wpawn, wbishop, wking, wknight, wqueen, wrook  // 흰(6~11)
};

struct chessPiece
{
    PieceType type;
    RECT      rect;
    bool      alive;
    bool      selected;
    int       moveCount;
};

struct AppData
{
    chessPiece pieces[32];
    bool       moveHint[8][8];
};

void* g_images[12];

// 이동 로그
struct MoveLog
{
    PieceType type;
    bool      white;
    int       fromX, fromY, toX, toY;
    bool      capture;
    char      from[3], to[3];
};
MoveLog g_moveLog[512];
int     g_moveLogCount = 0;

// ===================================================
// 함수 프로토타입
// ===================================================
bool IsWhite(PieceType t);                                                                          // 흰색 말인지 확인
void ClearMoveHints(AppData* app);                                                                  // 이동 힌트 초기화
void CalcMoveHints(AppData* app, int idx);                                                          // 이동 힌트 계산
int  GetSelectedPieceIndex(AppData* app);                                                           // 선택된 말 인덱스 반환
int  GetPieceAt(AppData* app, int x, int y);                                                        // 특정 위치의 말 인덱스 반환
void PixelToChess(int px, int py, char out[3]);                                                     // 픽셀 좌표를 체스 좌표로 변환
void AddMoveLog(PieceType type, bool white, int fromX, int fromY, int toX, int toY, bool capture); // 이동 로그 추가
unsigned long long NowMs();                                                                         // 현재 시간(ms) 반환
void SeedRng();                                                                                     // 난수 초기화
unsigned int NextRand();                                                                            // 다음 난수 반환
int  RandInt(int n);                                                                                // 0~n 사이 난수 반환
bool IsPawnPromotionSquare(const chessPiece& p);                                                    // 프로모션 칸인지 확인
void AutoPromoteBlackIfNeeded(AppData* app, int idx);                                               // 검은 폰 자동 퀸 승격
void SetupPromotionUI();                                                                            // 프로모션 UI 버튼 위치 설정
void StartPromotion(AppData* app, int idx, bool white);                                             // 프로모션 시작
void ApplyPromotionChoice(AppData* app, int choice);                                                // 프로모션 선택 적용
int  PieceValue(PieceType t);                                                                       // 말의 가치 반환
bool CanMovePawn(AppData* app, int idx, int toX, int toY);                                          // 폰 이동 가능 여부
bool CanMoveRook(AppData* app, int idx, int toX, int toY);                                          // 룩 이동 가능 여부
bool CanMoveBishop(AppData* app, int idx, int toX, int toY);                                        // 비숍 이동 가능 여부
bool CanMoveKnight(AppData* app, int idx, int toX, int toY);                                        // 나이트 이동 가능 여부
bool CanMoveQueen(AppData* app, int idx, int toX, int toY);                                         // 퀸 이동 가능 여부
bool CanMoveKing(AppData* app, int idx, int toX, int toY);                                          // 킹 이동 가능 여부 (캐슬링 포함)
bool CanMovePieceEx(AppData* app, int idx, int toX, int toY, bool forCheck);                        // 말 이동 가능 여부 (체크 판정용)
bool CanMovePiece(AppData* app, int idx, int toX, int toY);                                         // 말 이동 가능 여부
bool IsKingInCheck(AppData* app, bool whiteKing);                                                   // 킹 체크 여부 확인
bool CanMoveWhenInCheck(AppData* app, int idx, int toX, int toY);                                   // 체크 상태에서 이동 가능 여부
bool HasAnyLegalMove(AppData* app, bool white);                                                     // 합법적인 이동이 있는지 확인
bool IsCheckmate(AppData* app, bool whiteKing);                                                     // 체크메이트 여부 확인
bool IsStalemate(AppData* app, bool white);                                                         // 스테일메이트 여부 확인
void UpdateCheckState(AppData* app);                                                                // 체크/체크메이트/스테일메이트 상태 갱신
void MovePiece(AppData* app, int idx, int toX, int toY);                                            // 말 이동 실행
bool CapturePiece(AppData* app, int attackerIdx, int targetIdx);                                    // 말 캡처 실행
struct AIMove { int pieceIdx; int toX, toY; int targetIdx; };                                       // AI 이동 데이터 구조체
void AutoMoveBlack(AppData* app);                                                                   // 검은색 AI 이동 실행
void* LoadImageFromResource(int resourceId);                                                        // 리소스에서 이미지 로드
void LoadImages();                                                                                  // 체스말 이미지 로드
void CreatePiece(chessPiece& p, PieceType type, int x, int y);                                     // 말 생성
void InitChessPieces(AppData* app);                                                                 // 말 초기 배치
void ResetGame(AppData* app);                                                                       // 게임 리셋
void OnDraw(AppData* app);                                                                          // 화면 그리기
void SaveCurrentPosition(AppData* app);                                                             // 현재 국면 저장 (반복 판정용)
bool IsThreefoldRepetition(AppData* app);                                                           // 반복 무승부 판정
int  FlipX(int x);                                                                                  // 화면 X 좌표 뒤집기
int  FlipY(int y);                                                                                  // 화면 Y 좌표 뒤집기
// 네트워크 관련
void OnAcceptUser(CurrentServerNetworkData* ap_data);                                               // 클라이언트 접속 콜백
int  OnReceiveData(CurrentServerNetworkData* ap_data);                                              // 서버 데이터 수신 콜백
void OnCloseUser(CurrentServerNetworkData* ap_data);                                                // 클라이언트 접속 해제 콜백
void OnConnect(CurrentClientNetworkData* ap_data);                                                  // 서버 접속 성공 콜백
int  OnReceiveClientData(CurrentClientNetworkData* ap_data);                                        // 클라이언트 데이터 수신 콜백
void OnDisconnect(CurrentClientNetworkData* ap_data);                                               // 서버 접속 해제 콜백
void StartServer();                                                                                 // 서버 시작
void JoinServer(const char* ip);                                                                    // 서버 접속
void SendMoveToOpponent(int pieceIdx, int toX, int toY, int targetIdx, bool isCapture);             // 상대방에게 이동 전송

// ===================================================
// 뒤집기 좌표 변환
// ===================================================
int FlipX(int x) { return x; }
int FlipY(int y) { return g_boardFlipped ? (175 + 875 - y) : y; }

// ===================================================
// 리소스에서 이미지 로드
// ===================================================
void* LoadImageFromResource(int resourceId)
{
    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(resourceId), "PNG");
    if (!hRes) return NULL;
    HGLOBAL hData = LoadResource(NULL, hRes);
    if (!hData) return NULL;
    void* pData = LockResource(hData);
    DWORD size = SizeofResource(NULL, hRes);

    char tmpPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tmpPath);
    strcat_s(tmpPath, MAX_PATH, "tmpchess.png");

    FILE* f = NULL;
    fopen_s(&f, tmpPath, "wb");
    if (f) { fwrite(pData, 1, size, f); fclose(f); }

    return LoadImageGP(tmpPath);
}

// ===================================================
// 이미지 로드
// ===================================================
void LoadImages()
{
    g_images[bishop] = LoadImageFromResource(IDB_PNG1);   // 검은 비숍
    g_images[wbishop] = LoadImageFromResource(IDB_PNG2);   // 흰 비숍
    g_images[king] = LoadImageFromResource(IDB_PNG3);   // 검은 킹
    g_images[wking] = LoadImageFromResource(IDB_PNG4);   // 흰 킹
    g_images[knight] = LoadImageFromResource(IDB_PNG5);   // 검은 나이트
    g_images[wknight] = LoadImageFromResource(IDB_PNG6);   // 흰 나이트
    g_images[pawn] = LoadImageFromResource(IDB_PNG7);   // 검은 폰
    g_images[wpawn] = LoadImageFromResource(IDB_PNG8);   // 흰 폰
    g_images[queen] = LoadImageFromResource(IDB_PNG9);   // 검은 퀸
    g_images[wqueen] = LoadImageFromResource(IDB_PNG10);  // 흰 퀸
    g_images[rook] = LoadImageFromResource(IDB_PNG11);  // 검은 룩
    g_images[wrook] = LoadImageFromResource(IDB_PNG12);  // 흰 룩
}

// ===================================================
// 유틸
// ===================================================
bool IsWhite(PieceType t) { return t >= wpawn; }

void ClearMoveHints(AppData* app)
{
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++)
            app->moveHint[y][x] = false;
}

bool IsPawnPromotionSquare(const chessPiece& p)
{
    if (p.type == wpawn) return (p.rect.top == 175);
    if (p.type == pawn)  return (p.rect.top == 875);
    return false;
}

void AutoPromoteBlackIfNeeded(AppData* app, int idx)
{
    chessPiece& p = app->pieces[idx];
    if (!p.alive || p.type != pawn) return;
    if (IsPawnPromotionSquare(p)) p.type = queen;
}

int PieceValue(PieceType t)
{
    int base = (t >= wpawn) ? (t - wpawn) : t;
    switch (base)
    {
    case queen:  return 9;
    case rook:   return 5;
    case bishop: return 3;
    case knight: return 3;
    case pawn:   return 1;
    case king:   return 100;
    }
    return 0;
}

void SetupPromotionUI()
{
    int x0 = 1450, y0 = 350, w = 80, h = 60, gap = 10;
    for (int i = 0; i < 4; i++)
        g_promoBtns[i] = { x0, y0 + i * (h + gap), x0 + w, y0 + i * (h + gap) + h };
}

void StartPromotion(AppData* app, int idx, bool white)
{
    g_promotionActive = true;
    g_promoPawnIdx = idx;
    g_promoWhite = white;
    for (int i = 0; i < 32; i++) app->pieces[i].selected = false;
    ClearMoveHints(app);
}

void ApplyPromotionChoice(AppData* app, int choice)
{
    if (!g_promotionActive || g_promoPawnIdx < 0) return;
    chessPiece& p = app->pieces[g_promoPawnIdx];
    if (g_promoWhite)
    {
        PieceType tbl[4] = { wqueen, wrook, wknight, wbishop };
        p.type = tbl[choice];
    }
    else
    {
        PieceType tbl[4] = { queen, rook, knight, bishop };
        p.type = tbl[choice];
    }
    g_promotionActive = false;
    g_promoPawnIdx = -1;
}

void PixelToChess(int px, int py, char out[3])
{
    int file = (px - 625) / 100;
    int rank = (py - 175) / 100;
    if (file < 0) file = 0; if (file > 7) file = 7;
    if (rank < 0) rank = 0; if (rank > 7) rank = 7;
    out[0] = (char)('A' + file);
    out[1] = (char)('8' - rank);
    out[2] = 0;
}

unsigned long long NowMs() { return GetTickCount64(); }

void SeedRng()
{
    g_rng = (unsigned int)GetTickCount64();
    if (!g_rng) g_rng = 1;
}
unsigned int NextRand()
{
    g_rng = 1664525u * g_rng + 1013904223u;
    return g_rng;
}
int RandInt(int n) { return (n <= 0) ? 0 : (int)(NextRand() % (unsigned int)n); }

void AddMoveLog(PieceType type, bool white, int fromX, int fromY, int toX, int toY, bool capture)
{
    if (g_moveLogCount >= 512) return;
    MoveLog& m = g_moveLog[g_moveLogCount++];
    m.type = type; m.white = white;
    m.fromX = fromX; m.fromY = fromY;
    m.toX = toX;   m.toY = toY;
    m.capture = capture;
    PixelToChess(fromX, fromY, m.from);
    PixelToChess(toX, toY, m.to);
}

int GetSelectedPieceIndex(AppData* app)
{
    for (int i = 0; i < 32; i++)
        if (app->pieces[i].alive && app->pieces[i].selected) return i;
    return -1;
}

int GetPieceAt(AppData* app, int x, int y)
{
    for (int i = 0; i < 32; i++)
        if (app->pieces[i].alive &&
            app->pieces[i].rect.left == x &&
            app->pieces[i].rect.top == y) return i;
    return -1;
}

// ===================================================
// 말 배치
// ===================================================
void CreatePiece(chessPiece& p, PieceType type, int x, int y)
{
    p.type = type; p.alive = true; p.selected = false; p.moveCount = 0;
    SetRect(&p.rect, x, y, x + 100, y + 100);
}

void InitChessPieces(AppData* app)
{
    int i = 0;
    for (int x = 0; x < 8; x++) CreatePiece(app->pieces[i++], wpawn, 625 + 100 * x, 775);
    CreatePiece(app->pieces[i++], wrook, 625, 875);
    CreatePiece(app->pieces[i++], wknight, 725, 875);
    CreatePiece(app->pieces[i++], wbishop, 825, 875);
    CreatePiece(app->pieces[i++], wqueen, 925, 875);
    CreatePiece(app->pieces[i++], wking, 1025, 875);
    CreatePiece(app->pieces[i++], wbishop, 1125, 875);
    CreatePiece(app->pieces[i++], wknight, 1225, 875);
    CreatePiece(app->pieces[i++], wrook, 1325, 875);
    for (int x = 0; x < 8; x++) CreatePiece(app->pieces[i++], pawn, 625 + 100 * x, 275);
    CreatePiece(app->pieces[i++], rook, 625, 175);
    CreatePiece(app->pieces[i++], knight, 725, 175);
    CreatePiece(app->pieces[i++], bishop, 825, 175);
    CreatePiece(app->pieces[i++], queen, 925, 175);
    CreatePiece(app->pieces[i++], king, 1025, 175);
    CreatePiece(app->pieces[i++], bishop, 1125, 175);
    CreatePiece(app->pieces[i++], knight, 1225, 175);
    CreatePiece(app->pieces[i++], rook, 1325, 175);
}

// ===================================================
// 이동 규칙
// ===================================================
bool CanMovePawn(AppData* app, int idx, int toX, int toY)
{
    chessPiece& p = app->pieces[idx];
    int fromX = p.rect.left, fromY = p.rect.top;
    bool white = IsWhite(p.type);
    int dir = white ? -100 : 100;
    int dx = toX - fromX, dy = toY - fromY;
    int target = GetPieceAt(app, toX, toY);
    if (dx == 0 && dy == dir && target == -1) return true;
    if (dx == 0 && dy == dir * 2 && p.moveCount == 0)
    {
        int midY = fromY + dir;
        if (target == -1 && GetPieceAt(app, fromX, midY) == -1) return true;
    }
    if (abs(dx) == 100 && dy == dir)
    {
        if (target != -1 && IsWhite(app->pieces[target].type) != white) return true;
        if (toX == g_enPassantX && toY == g_enPassantY) return true;
    }
    return false;
}

bool CanMoveRook(AppData* app, int idx, int toX, int toY)
{
    chessPiece& p = app->pieces[idx];
    int fromX = p.rect.left, fromY = p.rect.top;
    int dx = toX - fromX, dy = toY - fromY;
    if (!(dx == 0 || dy == 0)) return false;
    int sx = (dx == 0) ? 0 : (dx > 0 ? 100 : -100);
    int sy = (dy == 0) ? 0 : (dy > 0 ? 100 : -100);
    int x = fromX + sx, y = fromY + sy;
    while (x != toX || y != toY)
    {
        if (GetPieceAt(app, x, y) != -1) return false;
        x += sx; y += sy;
    }
    return true;
}

bool CanMoveBishop(AppData* app, int idx, int toX, int toY)
{
    chessPiece& p = app->pieces[idx];
    int fromX = p.rect.left, fromY = p.rect.top;
    int dx = toX - fromX, dy = toY - fromY;
    if (abs(dx) != abs(dy)) return false;
    int sx = (dx > 0) ? 100 : -100;
    int sy = (dy > 0) ? 100 : -100;
    int x = fromX + sx, y = fromY + sy;
    while (x != toX || y != toY)
    {
        if (GetPieceAt(app, x, y) != -1) return false;
        x += sx; y += sy;
    }
    return true;
}

bool CanMoveKnight(AppData* app, int idx, int toX, int toY)
{
    chessPiece& p = app->pieces[idx];
    int dx = abs(toX - p.rect.left), dy = abs(toY - p.rect.top);
    return (dx == 200 && dy == 100) || (dx == 100 && dy == 200);
}

bool CanMoveQueen(AppData* app, int idx, int toX, int toY)
{
    chessPiece& p = app->pieces[idx];
    int dx = toX - p.rect.left, dy = toY - p.rect.top;
    if (dx == 0 || dy == 0) return CanMoveRook(app, idx, toX, toY);
    if (abs(dx) == abs(dy)) return CanMoveBishop(app, idx, toX, toY);
    return false;
}

bool CanMoveKing(AppData* app, int idx, int toX, int toY)
{
    chessPiece& p = app->pieces[idx];
    int fromX = p.rect.left, fromY = p.rect.top;
    int dx = abs(toX - fromX), dy = abs(toY - fromY);
    if (dx <= 100 && dy <= 100 && (dx + dy) > 0)
    {
        int target = GetPieceAt(app, toX, toY);
        if (target == -1) return true;
        return IsWhite(app->pieces[target].type) != IsWhite(p.type);
    }
    if (dy == 0 && dx == 200 && p.moveCount == 0)
    {
        bool white = IsWhite(p.type);
        int row = fromY;
        if (toX > fromX)
        {
            int rookIdx = GetPieceAt(app, fromX + 300, row);
            if (rookIdx == -1) return false;
            chessPiece& rook = app->pieces[rookIdx];
            if (rook.moveCount != 0) return false;
            if (IsWhite(rook.type) != white) return false;
            if (GetPieceAt(app, fromX + 100, row) != -1) return false;
            if (GetPieceAt(app, fromX + 200, row) != -1) return false;
            return true;
        }
        else
        {
            int rookIdx = GetPieceAt(app, fromX - 400, row);
            if (rookIdx == -1) return false;
            chessPiece& rook = app->pieces[rookIdx];
            if (rook.moveCount != 0) return false;
            if (IsWhite(rook.type) != white) return false;
            if (GetPieceAt(app, fromX - 100, row) != -1) return false;
            if (GetPieceAt(app, fromX - 200, row) != -1) return false;
            if (GetPieceAt(app, fromX - 300, row) != -1) return false;
            return true;
        }
    }
    return false;
}

bool CanMovePieceEx(AppData* app, int idx, int toX, int toY, bool forCheck)
{
    chessPiece& p = app->pieces[idx];
    if (p.rect.left == toX && p.rect.top == toY) return false;
    int target = GetPieceAt(app, toX, toY);
    if (target != -1)
    {
        if (IsWhite(app->pieces[target].type) == IsWhite(p.type)) return false;
        if (!forCheck)
        {
            PieceType tt = app->pieces[target].type;
            if (tt == king || tt == wking) return false;
        }
    }
    switch (p.type)
    {
    case pawn:   case wpawn:   return CanMovePawn(app, idx, toX, toY);
    case rook:   case wrook:   return CanMoveRook(app, idx, toX, toY);
    case bishop: case wbishop: return CanMoveBishop(app, idx, toX, toY);
    case knight: case wknight: return CanMoveKnight(app, idx, toX, toY);
    case queen:  case wqueen:  return CanMoveQueen(app, idx, toX, toY);
    case king:   case wking:   return CanMoveKing(app, idx, toX, toY);
    }
    return false;
}

bool CanMovePiece(AppData* app, int idx, int toX, int toY)
{
    return CanMovePieceEx(app, idx, toX, toY, false);
}

// ===================================================
// 체크 판정
// ===================================================
bool IsKingInCheck(AppData* app, bool whiteKing)
{
    int kingIdx = -1;
    for (int i = 0; i < 32; i++)
    {
        if (!app->pieces[i].alive) continue;
        if (whiteKing && app->pieces[i].type == wking) { kingIdx = i; break; }
        if (!whiteKing && app->pieces[i].type == king) { kingIdx = i; break; }
    }
    if (kingIdx == -1) { g_gameOver = true; g_winner = whiteKing ? 0 : 1; return true; }
    int kx = app->pieces[kingIdx].rect.left;
    int ky = app->pieces[kingIdx].rect.top;
    for (int i = 0; i < 32; i++)
    {
        if (!app->pieces[i].alive) continue;
        if (IsWhite(app->pieces[i].type) == whiteKing) continue;
        if (CanMovePieceEx(app, i, kx, ky, true)) return true;
    }
    return false;
}

bool CanMoveWhenInCheck(AppData* app, int idx, int toX, int toY)
{
    chessPiece backup = app->pieces[idx];
    int target = GetPieceAt(app, toX, toY);
    int epCaptureIdx = -1;
    if ((app->pieces[idx].type == pawn || app->pieces[idx].type == wpawn) &&
        toX == g_enPassantX && toY == g_enPassantY && target == -1)
    {
        bool white = IsWhite(app->pieces[idx].type);
        int captureY = toY + (white ? 100 : -100);
        epCaptureIdx = GetPieceAt(app, toX, captureY);
    }
    chessPiece targetBackup; bool hasTarget = false;
    if (target != -1) { targetBackup = app->pieces[target]; hasTarget = true; }
    chessPiece epBackup; bool hasEp = false;
    if (epCaptureIdx != -1) { epBackup = app->pieces[epCaptureIdx]; hasEp = true; }
    app->pieces[idx].rect.left = toX;
    app->pieces[idx].rect.top = toY;
    app->pieces[idx].rect.right = toX + 100;
    app->pieces[idx].rect.bottom = toY + 100;
    if (hasTarget) app->pieces[target].alive = false;
    if (hasEp)     app->pieces[epCaptureIdx].alive = false;
    bool inCheck = IsKingInCheck(app, IsWhite(backup.type));
    app->pieces[idx] = backup;
    if (hasTarget) app->pieces[target] = targetBackup;
    if (hasEp)     app->pieces[epCaptureIdx] = epBackup;
    return !inCheck;
}

bool HasAnyLegalMove(AppData* app, bool white)
{
    for (int i = 0; i < 32; i++)
    {
        chessPiece& p = app->pieces[i];
        if (!p.alive || IsWhite(p.type) != white) continue;
        for (int y = 0; y < 8; y++)
            for (int x = 0; x < 8; x++)
            {
                int bx = 625 + x * 100, by = 175 + y * 100;
                if (CanMovePiece(app, i, bx, by) && CanMoveWhenInCheck(app, i, bx, by))
                    return true;
            }
    }
    return false;
}

bool IsCheckmate(AppData* app, bool whiteKing)
{
    if (!IsKingInCheck(app, whiteKing)) return false;
    return !HasAnyLegalMove(app, whiteKing);
}

bool IsStalemate(AppData* app, bool white)
{
    if (IsKingInCheck(app, white)) return false;
    return !HasAnyLegalMove(app, white);
}

void UpdateCheckState(AppData* app)
{
    if (g_gameOver) return;
    g_whiteInCheck = IsKingInCheck(app, true);
    g_blackInCheck = IsKingInCheck(app, false);
    g_whiteInCheckmate = IsCheckmate(app, true);
    g_blackInCheckmate = IsCheckmate(app, false);
    g_stalemate = IsStalemate(app, g_whiteTurn);
    SaveCurrentPosition(app);
    if (IsThreefoldRepetition(app)) { g_gameOver = true; g_winner = 2; return; }
    if (g_whiteInCheckmate) { g_gameOver = true; g_winner = 0; }
    if (g_blackInCheckmate) { g_gameOver = true; g_winner = 1; }
    if (g_stalemate) { g_gameOver = true; g_winner = 2; }
}

void SaveCurrentPosition(AppData* app)
{
    if (g_positionCount >= 200) return;
    char* pos = g_positionHistory[g_positionCount++];
    int idx = 0;
    for (int i = 0; i < 32; i++)
    {
        chessPiece& p = app->pieces[i];
        if (!p.alive) { pos[idx++] = 0; pos[idx++] = 0; pos[idx++] = 0; continue; }
        pos[idx++] = (char)p.type;
        pos[idx++] = (char)(p.rect.left / 100);
        pos[idx++] = (char)(p.rect.top / 100);
    }
}

bool IsThreefoldRepetition(AppData* app)
{
    char cur[32 * 3];
    int idx = 0;
    for (int i = 0; i < 32; i++)
    {
        chessPiece& p = app->pieces[i];
        if (!p.alive) { cur[idx++] = 0; cur[idx++] = 0; cur[idx++] = 0; continue; }
        cur[idx++] = (char)p.type;
        cur[idx++] = (char)(p.rect.left / 100);
        cur[idx++] = (char)(p.rect.top / 100);
    }
    int limit = 3;
    if (g_difficulty == DIF_EASY || g_difficulty == DIF_NORMAL) limit = 5;
    int count = 0;
    for (int i = 0; i < g_positionCount; i++)
    {
        if (memcmp(g_positionHistory[i], cur, 32 * 3) == 0) count++;
        if (count >= limit) return true;
    }
    return false;
}

// ===================================================
// 이동 힌트 계산
// ===================================================
void CalcMoveHints(AppData* app, int idx)
{
    ClearMoveHints(app);
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++)
        {
            int bx = 625 + x * 100, by = 175 + y * 100;
            if (!CanMovePiece(app, idx, bx, by)) continue;
            if (!CanMoveWhenInCheck(app, idx, bx, by)) continue;
            app->moveHint[y][x] = true;
        }
}

// ===================================================
// 이동 실행
// ===================================================
void MovePiece(AppData* app, int idx, int toX, int toY)
{
    chessPiece& p = app->pieces[idx];
    int fromX = p.rect.left, fromY = p.rect.top;
    bool white = IsWhite(p.type);
    AddMoveLog(p.type, white, fromX, fromY, toX, toY, false);
    if ((p.type == pawn || p.type == wpawn) && toX == g_enPassantX && toY == g_enPassantY)
    {
        int captureY = toY + (white ? 100 : -100);
        int epIdx = GetPieceAt(app, toX, captureY);
        if (epIdx != -1) app->pieces[epIdx].alive = false;
    }
    g_enPassantX = -1; g_enPassantY = -1;
    if ((p.type == wpawn || p.type == pawn) && abs(toY - fromY) == 200)
    {
        g_enPassantX = toX;
        g_enPassantY = (fromY + toY) / 2;
    }
    if ((p.type == wking || p.type == king) && abs(toX - fromX) == 200)
    {
        int row = fromY;
        if (toX > fromX)
        {
            int rookIdx = GetPieceAt(app, fromX + 300, row);
            if (rookIdx != -1) { SetRect(&app->pieces[rookIdx].rect, toX - 100, row, toX, row + 100); app->pieces[rookIdx].moveCount++; }
        }
        else
        {
            int rookIdx = GetPieceAt(app, fromX - 400, row);
            if (rookIdx != -1) { SetRect(&app->pieces[rookIdx].rect, toX + 100, row, toX + 200, row + 100); app->pieces[rookIdx].moveCount++; }
        }
    }
    SetRect(&p.rect, toX, toY, toX + 100, toY + 100);
    p.moveCount++;
    if (IsPawnPromotionSquare(p))
    {
        if (p.type == wpawn)      StartPromotion(app, idx, true);
        else if (p.type == pawn)  AutoPromoteBlackIfNeeded(app, idx);
    }
    if (g_promotionActive) return;
    g_whiteTurn = !g_whiteTurn;
    if (!g_whiteTurn) g_whiteAiPlayedThisTurn = false;
    // ✅ 오프라인 PVP - 차례 바뀔 때 뒤집기
    if (g_difficulty == DIF_PVP)
        g_boardFlipped = !g_whiteTurn;
    if (g_difficulty == DIF_ONLINE_PVP && g_isConnected)
        SendMoveToOpponent(idx, toX, toY, -1, false);
}

bool CapturePiece(AppData* app, int attackerIdx, int targetIdx)
{
    chessPiece& attacker = app->pieces[attackerIdx];
    chessPiece& target = app->pieces[targetIdx];
    if (target.type == king || target.type == wking) return false;
    int fromX = attacker.rect.left, fromY = attacker.rect.top;
    int toX = target.rect.left, toY = target.rect.top;
    if ((attacker.type == wpawn || attacker.type == pawn) && abs(toY - fromY) != 200)
        g_enPassantX = g_enPassantY = -1;
    else if (attacker.type != wpawn && attacker.type != pawn)
        g_enPassantX = g_enPassantY = -1;
    AddMoveLog(attacker.type, IsWhite(attacker.type), fromX, fromY, toX, toY, true);
    target.alive = target.selected = false;
    SetRect(&attacker.rect, toX, toY, toX + 100, toY + 100);
    attacker.moveCount++;
    if (IsPawnPromotionSquare(attacker))
    {
        if (attacker.type == wpawn)      StartPromotion(app, attackerIdx, true);
        else if (attacker.type == pawn)  AutoPromoteBlackIfNeeded(app, attackerIdx);
    }
    if (g_promotionActive) return true;
    g_whiteTurn = !g_whiteTurn;
    if (!g_whiteTurn) g_whiteAiPlayedThisTurn = false;
    // ✅ 오프라인 PVP - 차례 바뀔 때 뒤집기
    if (g_difficulty == DIF_PVP)
        g_boardFlipped = !g_whiteTurn;
    if (g_difficulty == DIF_ONLINE_PVP && g_isConnected)
        SendMoveToOpponent(attackerIdx, toX, toY, targetIdx, true);
    return true;
}

// ===================================================
// 검은색 AI
// ===================================================
int CollectBlackLegalMoves(AppData* app, AIMove outMoves[], int maxMoves)
{
    int count = 0;
    for (int i = 0; i < 32; i++)
    {
        chessPiece& p = app->pieces[i];
        if (!p.alive || IsWhite(p.type)) continue;
        for (int y = 0; y < 8; y++)
            for (int x = 0; x < 8; x++)
            {
                int bx = 625 + x * 100, by = 175 + y * 100;
                if (!CanMovePiece(app, i, bx, by)) continue;
                if (!CanMoveWhenInCheck(app, i, bx, by)) continue;
                int t = GetPieceAt(app, bx, by);
                if (t != -1 && (app->pieces[t].type == king || app->pieces[t].type == wking)) continue;
                if (count < maxMoves) outMoves[count++] = { i, bx, by, t };
            }
    }
    return count;
}

// ===================================================
// 이동 패킷 구조
// ===================================================
struct MovePacket
{
    int  pieceIdx;
    int  toX, toY;
    int  targetIdx;
    bool isCapture;
};

// ===================================================
// 서버 콜백
// ===================================================
void OnAcceptUser(CurrentServerNetworkData* ap_data)
{
    g_isConnected = true;
    AppData* app = (AppData*)GetAppData();
    g_boardFlipped = false;  // 서버(방장)는 뒤집기 없음
    ResetGame(app);
    g_scene = SCENE_GAME;
    OnDraw(app);
}

int OnReceiveData(CurrentServerNetworkData* ap_data)
{
    if (ap_data->m_net_msg_id == 1)
    {
        MovePacket* pkt = (MovePacket*)ap_data->mp_net_body_data;
        AppData* app = (AppData*)GetAppData();
        if (pkt->isCapture) CapturePiece(app, pkt->pieceIdx, pkt->targetIdx);
        else                MovePiece(app, pkt->pieceIdx, pkt->toX, pkt->toY);
        UpdateCheckState(app);
        OnDraw(app);
    }
    return 1;
}

void OnCloseUser(CurrentServerNetworkData* ap_data) { g_isConnected = false; }

// ===================================================
// 클라이언트 콜백
// ===================================================
void OnConnect(CurrentClientNetworkData* ap_data)
{
    g_isConnected = true;
    AppData* app = (AppData*)GetAppData();
    g_boardFlipped = true;  // ✅ 클라이언트는 항상 뒤집기
    ResetGame(app);
    g_scene = SCENE_GAME;
    OnDraw(app);
}

int OnReceiveClientData(CurrentClientNetworkData* ap_data)
{
    if (ap_data->m_net_msg_id == 1)
    {
        MovePacket* pkt = (MovePacket*)ap_data->mp_net_body_data;
        AppData* app = (AppData*)GetAppData();
        if (pkt->isCapture) CapturePiece(app, pkt->pieceIdx, pkt->targetIdx);
        else                MovePiece(app, pkt->pieceIdx, pkt->toX, pkt->toY);
        UpdateCheckState(app);
        OnDraw(app);
    }
    return 1;
}

void OnDisconnect(CurrentClientNetworkData* ap_data) { g_isConnected = false; }

// ===================================================
// 서버/클라이언트 시작
// ===================================================
void StartServer()
{
    StartSocketSystem();
    g_serverSocket = CreateServerSocket(0, OnAcceptUser, OnReceiveData, OnCloseUser, 0, 2);
    StartListenService(g_serverSocket, "0.0.0.0", 9000);
}

void JoinServer(const char* ip)
{
    StartSocketSystem();
    g_clientSocket = CreateClientSocket(OnConnect, OnReceiveClientData, OnDisconnect);
    ConnectToServer(g_clientSocket, ip, 9000);
}

void SendMoveToOpponent(int pieceIdx, int toX, int toY, int targetIdx, bool isCapture)
{
    MovePacket pkt = { pieceIdx, toX, toY, targetIdx, isCapture };
    if (g_isHost) SendFrameDataToClient(g_serverSocket, 0, 1, &pkt, sizeof(pkt));
    else          SendFrameDataToServer(g_clientSocket, 1, &pkt, sizeof(pkt));
}

// ===================================================
// AI
// ===================================================
void AutoMoveBlack(AppData* app)
{
    if (g_gameOver || g_whiteTurn || g_whiteAiPlayedThisTurn || g_promotionActive) return;
    AIMove moves[1024];
    int count = 0;
    for (int i = 0; i < 32; i++)
    {
        chessPiece& p = app->pieces[i];
        if (!p.alive || IsWhite(p.type)) continue;
        for (int y = 0; y < 8; y++)
            for (int x = 0; x < 8; x++)
            {
                int bx = 625 + x * 100, by = 175 + y * 100;
                if (!CanMovePiece(app, i, bx, by)) continue;
                if (!CanMoveWhenInCheck(app, i, bx, by)) continue;
                int t = GetPieceAt(app, bx, by);
                if (t != -1 && (app->pieces[t].type == king || app->pieces[t].type == wking)) continue;
                if (count < 1024) moves[count++] = { i, bx, by, t };
            }
    }
    if (count <= 0) { g_whiteAiPlayedThisTurn = true; return; }

    if (g_difficulty == DIF_EASY)
    {
        AIMove m = moves[RandInt(count)];
        if (m.targetIdx != -1) CapturePiece(app, m.pieceIdx, m.targetIdx);
        else                   MovePiece(app, m.pieceIdx, m.toX, m.toY);
        UpdateCheckState(app);
        for (int i = 0; i < 32; i++) app->pieces[i].selected = false;
        ClearMoveHints(app);
        return;
    }

    auto PickNormal = [&]() -> AIMove
        {
            int bestIdx = -1, bestScore = -999999;
            for (int i = 0; i < count; i++)
            {
                int score = 0;
                if (moves[i].targetIdx == -1) continue;
                PieceValue(app->pieces[moves[i].targetIdx].type) * 100 + RandInt(100);
                chessPiece& mp = app->pieces[moves[i].pieceIdx];
                if (mp.type == queen) score -= 150;
                if (mp.type == king)  score -= 300;
                if (score > bestScore) { bestScore = score; bestIdx = i; }
            }
            return (bestIdx != -1) ? moves[bestIdx] : moves[RandInt(count)];
        };

    auto ScoreGivesCheck = [&](const AIMove& mv) -> int
        {
            chessPiece bk = app->pieces[mv.pieceIdx];
            chessPiece tk; bool hasTk = mv.targetIdx != -1;
            if (hasTk) tk = app->pieces[mv.targetIdx];
            SetRect(&app->pieces[mv.pieceIdx].rect, mv.toX, mv.toY, mv.toX + 100, mv.toY + 100);
            if (hasTk) app->pieces[mv.targetIdx].alive = false;
            bool gives = IsKingInCheck(app, true);
            app->pieces[mv.pieceIdx] = bk;
            if (hasTk) app->pieces[mv.targetIdx] = tk;
            return gives ? 1 : 0;
        };

    auto ScoreHanging = [&](const AIMove& mv) -> int
        {
            chessPiece bk = app->pieces[mv.pieceIdx];
            chessPiece tk; bool hasTk = mv.targetIdx != -1;
            if (hasTk) tk = app->pieces[mv.targetIdx];
            SetRect(&app->pieces[mv.pieceIdx].rect, mv.toX, mv.toY, mv.toX + 100, mv.toY + 100);
            if (hasTk) app->pieces[mv.targetIdx].alive = false;
            bool attacked = false;
            for (int i = 0; i < 32; i++)
            {
                if (!app->pieces[i].alive || !IsWhite(app->pieces[i].type)) continue;
                if (CanMovePieceEx(app, i, mv.toX, mv.toY, true)) { attacked = true; break; }
            }
            app->pieces[mv.pieceIdx] = bk;
            if (hasTk) app->pieces[mv.targetIdx] = tk;
            return attacked ? 1 : 0;
        };

    AIMove chosen;
    if (g_difficulty == DIF_NORMAL) chosen = PickNormal();
    else if (g_difficulty == DIF_HARD)
    {
        int bestScore = -999999, bestIdx = 0;
        for (int i = 0; i < count; i++)
        {
            int score = 0;
            if (moves[i].targetIdx != -1) score += PieceValue(app->pieces[moves[i].targetIdx].type) * 100;
            score += ScoreGivesCheck(moves[i]) * 60;
            chessPiece& mp = app->pieces[moves[i].pieceIdx];
            if (mp.type == queen) score -= 150;
            if (mp.type == king)  score -= 300;
            score += RandInt(10);
            if (score > bestScore) { bestScore = score; bestIdx = i; }
        }
        chosen = moves[bestIdx];
    }
    else
    {
        int bestScore = -999999, bestIdx = 0;
        for (int i = 0; i < count; i++)
        {
            int score = 0;
            if (moves[i].targetIdx != -1) score += PieceValue(app->pieces[moves[i].targetIdx].type) * 300;
            score += ScoreGivesCheck(moves[i]) * 200;
            score -= ScoreHanging(moves[i]) * 200;
            int cx = moves[i].toX, cy = moves[i].toY;
            if (cx >= 725 && cx <= 1125 && cy >= 275 && cy <= 675) score += 80;
            chessPiece& mp = app->pieces[moves[i].pieceIdx];
            int advance = moves[i].toY - mp.rect.top;
            score += advance / 10;
            if (mp.type == queen) score -= 150;
            if (mp.type == king)  score -= 300;
            score += RandInt(1);
            if (score > bestScore) { bestScore = score; bestIdx = i; }
        }
        chosen = moves[bestIdx];
    }

    if (chosen.targetIdx != -1) CapturePiece(app, chosen.pieceIdx, chosen.targetIdx);
    else                        MovePiece(app, chosen.pieceIdx, chosen.toX, chosen.toY);
    UpdateCheckState(app);
    for (int i = 0; i < 32; i++) app->pieces[i].selected = false;
    ClearMoveHints(app);
}

// ===================================================
// 그리기
// ===================================================
void OnDraw(AppData* app)
{
    Clear(0, RGB(180, 180, 180));

    // TITLE 화면
    if (g_scene == SCENE_TITLE)
    {
        Clear(0, RGB(40, 40, 60));
        SelectFontObject("Malgun Gothic", 80, 1);
        SetTextColor(RGB(255, 255, 255));
        TextOut(860, 480, "체스");
        SelectBrushObject(RGB(200, 220, 255));
        SelectPenObject(RGB(0, 0, 0), 2, PS_SOLID);
        Rectangle(g_btnStart.left, g_btnStart.top, g_btnStart.right, g_btnStart.bottom);
        SelectBrushObject(NULL_BRUSH); SelectPenObject(NULL_PEN);
        SelectFontObject("Malgun Gothic", 22, 1);
        SetTextColor(RGB(0, 0, 0));
        TextOut(g_btnStart.left + 75, g_btnStart.top + 20, "시작");
        SelectBrushObject(RGB(255, 180, 180));
        SelectPenObject(RGB(0, 0, 0), 2, PS_SOLID);
        Rectangle(g_btnExit.left, g_btnExit.top, g_btnExit.right, g_btnExit.bottom);
        SelectBrushObject(NULL_BRUSH); SelectPenObject(NULL_PEN);
        SelectFontObject("Malgun Gothic", 22, 1);
        SetTextColor(RGB(0, 0, 0));
        TextOut(g_btnExit.left + 85, g_btnExit.top + 20, "EXIT");
        SelectFontObject("Malgun Gothic", 16, 0);
        SetTextColor(RGB(0, 0, 0));
        ShowDisplay();
        return;
    }

    // 난이도 선택 화면
    if (g_scene == SCENE_DIFFICULTY)
    {
        Clear(0, RGB(40, 60, 40));
        SelectFontObject("Malgun Gothic", 40, 1);
        SetTextColor(RGB(255, 255, 255));
        TextOut(820, 380, "난이도 선택");
        auto DrawBtn = [&](RECT r, const char* t)
            {
                SelectBrushObject(RGB(220, 220, 220));
                SelectPenObject(RGB(0, 0, 0), 2, PS_SOLID);
                Rectangle(r.left, r.top, r.right, r.bottom);
                SelectBrushObject(NULL_BRUSH); SelectPenObject(NULL_PEN);
                SelectFontObject("Malgun Gothic", 20, 1);
                SetTextColor(RGB(0, 0, 0));
                TextOut(r.left + 60, r.top + 22, t);
            };
        DrawBtn(g_btnEasy, "쉬움");
        DrawBtn(g_btnNormal, "보통");
        DrawBtn(g_btnLocalPVP, "오프라인대전");
        DrawBtn(g_btnHard, "어려움");
        DrawBtn(g_btnHardcore, "매우 어려움");
        DrawBtn(g_btnOnlinePVP, "온라인대전");
        DrawBtn(g_btnBack, "뒤로가기");
        SelectFontObject("Malgun Gothic", 16, 0);
        SetTextColor(RGB(0, 0, 0));
        ShowDisplay();
        return;
    }

    // 로비 화면
    if (g_scene == SCENE_LOBBY)
    {
        Clear(0, RGB(30, 30, 50));
        SelectFontObject("Malgun Gothic", 40, 1);
        SetTextColor(RGB(255, 255, 255));
        TextOut(820, 380, "온라인 PVP");
        auto DrawBtn = [&](RECT r, const char* t)
            {
                SelectBrushObject(RGB(220, 220, 220));
                SelectPenObject(RGB(0, 0, 0), 2, PS_SOLID);
                Rectangle(r.left, r.top, r.right, r.bottom);
                SelectBrushObject(NULL_BRUSH); SelectPenObject(NULL_PEN);
                SelectFontObject("Malgun Gothic", 20, 1);
                SetTextColor(RGB(0, 0, 0));
                TextOut(r.left + 50, r.top + 22, t);
            };
        DrawBtn(g_btnHost, "방 만들기");
        DrawBtn(g_btnJoin, "방 참가");
        DrawBtn(g_btnLobbyBack, "뒤로가기");
        SelectFontObject("Malgun Gothic", 16, 0);
        SetTextColor(RGB(0, 0, 0));
        ShowDisplay();
        return;
    }

    // 대기 화면
    if (g_scene == SCENE_WAITING)
    {
        Clear(0, RGB(30, 30, 50));
        SelectFontObject("Malgun Gothic", 30, 1);
        SetTextColor(RGB(255, 255, 255));
        TextOut(750, 350, "상대방을 기다리는 중...");
        char ipList[10][16];
        int ipCount = GetLocalNetworkAddress(ipList);
        char ipText[64] = "내 IP: ";
        if (ipCount > 0) strcat_s(ipText, sizeof(ipText), ipList[0]);
        else             strcat_s(ipText, sizeof(ipText), "IP를 찾을 수 없음");
        SelectFontObject("Malgun Gothic", 24, 1);
        SetTextColor(RGB(100, 255, 100));
        TextOut(750, 450, ipText);
        SelectFontObject("Malgun Gothic", 18, 0);
        SetTextColor(RGB(200, 200, 200));
        TextOut(750, 520, "위 IP를 상대방에게 알려주세요.");
        TextOut(750, 560, "포트: 9000");
        SelectBrushObject(RGB(180, 100, 100));
        SelectPenObject(RGB(0, 0, 0), 2, PS_SOLID);
        Rectangle(g_btnWaitingBack.left, g_btnWaitingBack.top, g_btnWaitingBack.right, g_btnWaitingBack.bottom);
        SelectBrushObject(NULL_BRUSH); SelectPenObject(NULL_PEN);
        SelectFontObject("Malgun Gothic", 20, 1);
        SetTextColor(RGB(255, 255, 255));
        TextOut(g_btnWaitingBack.left + 60, g_btnWaitingBack.top + 22, "뒤로가기");
        SelectFontObject("Malgun Gothic", 16, 0);
        SetTextColor(RGB(0, 0, 0));
        ShowDisplay();
        return;
    }

    // IP 입력 화면
    if (g_scene == SCENE_JOIN)
    {
        Clear(0, RGB(30, 30, 50));
        SelectFontObject("Malgun Gothic", 30, 1);
        SetTextColor(RGB(255, 255, 255));
        TextOut(750, 350, "방 참가");
        SelectFontObject("Malgun Gothic", 20, 0);
        SetTextColor(RGB(200, 200, 200));
        TextOut(750, 450, "상대방 IP 주소 입력:");
        SelectBrushObject(RGB(100, 180, 100));
        SelectPenObject(RGB(0, 0, 0), 2, PS_SOLID);
        Rectangle(g_btnConnect.left, g_btnConnect.top, g_btnConnect.right, g_btnConnect.bottom);
        SelectBrushObject(NULL_BRUSH); SelectPenObject(NULL_PEN);
        SelectFontObject("Malgun Gothic", 20, 1);
        SetTextColor(RGB(0, 0, 0));
        TextOut(g_btnConnect.left + 90, g_btnConnect.top + 22, "접속");
        SelectBrushObject(RGB(180, 100, 100));
        SelectPenObject(RGB(0, 0, 0), 2, PS_SOLID);
        Rectangle(g_btnJoinBack.left, g_btnJoinBack.top, g_btnJoinBack.right, g_btnJoinBack.bottom);
        SelectBrushObject(NULL_BRUSH); SelectPenObject(NULL_PEN);
        SelectFontObject("Malgun Gothic", 20, 1);
        SetTextColor(RGB(255, 255, 255));
        TextOut(g_btnJoinBack.left + 60, g_btnJoinBack.top + 22, "뒤로가기");
        SelectFontObject("Malgun Gothic", 16, 0);
        SetTextColor(RGB(0, 0, 0));
        ShowDisplay();
        return;
    }

    // 설정 화면
    if (g_scene == SCENE_SETTINGS)
    {
        Clear(0, RGB(50, 50, 50));
        SelectFontObject("Malgun Gothic", 40, 1);
        SetTextColor(RGB(255, 255, 255));
        TextOut(860, 480, "SETTINGS");
        auto DrawBtn = [&](RECT r, const char* t)
            {
                SelectBrushObject(RGB(220, 220, 220));
                SelectPenObject(RGB(0, 0, 0), 2, PS_SOLID);
                Rectangle(r.left, r.top, r.right, r.bottom);
                SelectBrushObject(NULL_BRUSH); SelectPenObject(NULL_PEN);
                SelectFontObject("Malgun Gothic", 18, 0);
                SetTextColor(RGB(0, 0, 0));
                TextOut(r.left + 20, r.top + 22, t);
            };
        DrawBtn(g_btnSetBack, "게임화면으로 돌아가기");
        DrawBtn(g_btnSetReselect, "뒤로가기(난이도 선택)");
        DrawBtn(g_btnSetRestart, "처음부터 다시하기");
        SelectFontObject("Malgun Gothic", 16, 0);
        SetTextColor(RGB(0, 0, 0));
        ShowDisplay();
        return;
    }

    // ── 게임 화면 ──
    SelectPenObject(NULL_PEN);
    SelectBrushObject(NULL_BRUSH);

    // 좌표 표시
    SelectFontObject("Malgun Gothic", 32, 0);
    SetTextColor(RGB(0, 0, 0));
    for (int x = 0; x < 8; x++)
    {
        char t[2] = { (char)('A' + x), 0 };
        TextOut(625 + x * 100 + 20, 175 + 8 * 100, t);
        TextOut(625 + x * 100 + 25, 175 - 55, t);
    }
    for (int y = 0; y < 8; y++)
    {
        char t[2] = { (char)('8' - y), 0 };
        TextOut(625 + 8 * 100, 175 + y * 100 + 10, t);
        TextOut(625 - 50, 175 + y * 100 + 10, t);
    }

    // 이동 로그 표
    SelectBrushObject(RGB(255, 255, 255));
    Rectangle(100, 100, 300, 800);
    SelectPenObject(RGB(0, 0, 0));
    for (int y = 2; y < 16; y++) Line(100, y * 50, 300, y * 50);
    SelectPenObject(NULL_PEN);

    // 체스판
    SelectBrushObject(RGB(255, 255, 255));
    for (int x = 0; x < 4; x++)
    {
        for (int y = 0; y < 4; y++) Rectangle(600 + 200 * x, 150 + 200 * y, 700 + 200 * x, 250 + 200 * y);
        for (int y = 0; y < 4; y++) Rectangle(700 + 200 * x, 250 + 200 * y, 800 + 200 * x, 350 + 200 * y);
    }
    SelectBrushObject(RGB(0, 180, 0));
    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 4; x++) Rectangle(700 + 200 * x, 150 + 200 * y, 800 + 200 * x, 250 + 200 * y);
        for (int x = 0; x < 4; x++) Rectangle(600 + 200 * x, 250 + 200 * y, 700 + 200 * x, 350 + 200 * y);
    }
    SelectBrushObject(NULL_BRUSH);

    // 턴 표시
    if (g_whiteTurn) SelectBrushObject(RGB(240, 240, 240));
    else             SelectBrushObject(RGB(30, 30, 30));
    SelectPenObject(RGB(0, 0, 0), 2, PS_SOLID);
    Rectangle(100, 50, 300, 90);
    SelectBrushObject(NULL_BRUSH); SelectPenObject(NULL_PEN);
    SelectFontObject("Malgun Gothic", 16, 1);
    SetTextColor(g_whiteTurn ? RGB(0, 0, 0) : RGB(255, 255, 255));
    TextOut(120, 62, g_whiteTurn ? "흰색 차례" : "검은 차례");

    // 체크 상태
    SelectFontObject("Malgun Gothic", 16, 1);
    SetTextColor(RGB(220, 50, 50));
    if (g_whiteInCheck && !g_whiteInCheckmate) TextOut(320, 62, "흰색 체크!");
    if (g_blackInCheck && !g_blackInCheckmate) TextOut(320, 62, "검은색 체크!");

    // 프로모션 UI
    if (g_promotionActive)
    {
        SelectFontObject("Malgun Gothic", 18, 1);
        SetTextColor(RGB(255, 255, 255));
        TextOut(1450, 290, "PROMOTION");
        for (int i = 0; i < 4; i++)
        {
            SelectBrushObject(RGB(220, 220, 220));
            SelectPenObject(RGB(0, 0, 0), 2, PS_SOLID);
            Rectangle(g_promoBtns[i].left, g_promoBtns[i].top, g_promoBtns[i].right, g_promoBtns[i].bottom);
            SelectBrushObject(NULL_BRUSH); SelectPenObject(NULL_PEN);
            SelectFontObject("Malgun Gothic", 20, 1);
            SetTextColor(RGB(0, 0, 0));
            TextOut(g_promoBtns[i].left + 28, g_promoBtns[i].top + 15, g_promoText[i]);
        }
    }

    // BLACK THINK 표시
    if (g_showWhiteThink && g_difficulty != DIF_PVP && g_difficulty != DIF_ONLINE_PVP)
    {
        SelectFontObject("Malgun Gothic", 18, 1);
        SetTextColor(RGB(255, 220, 50));
        TextOut(1380, 100, "Black THINK...");
    }

    // 이동 힌트 표시 (✅ 뒤집기 적용)
    int sel = GetSelectedPieceIndex(app);
    if (sel != -1)
    {
        for (int y = 0; y < 8; y++)
        {
            for (int x = 0; x < 8; x++)
            {
                if (!app->moveHint[y][x]) continue;
                int bx = FlipX(625 + x * 100);
                int by = FlipY(175 + y * 100);
                int cx = bx + 25, cy = by + 25;
                int t = GetPieceAt(app, 625 + x * 100, 175 + y * 100);
                if (t == -1)
                {
                    SelectBrushObject(RGB(0, 0, 0));
                    SelectPenObject(NULL_PEN);
                    Ellipse(cx - 18, cy - 18, cx + 18, cy + 18);
                }
                else
                {
                    SelectBrushObject(RGB(255, 80, 80));
                    SelectPenObject(NULL_PEN);
                    Rectangle(bx - 25, by - 25, bx + 75, by + 75);
                }
            }
        }
    }
    SelectBrushObject(NULL_BRUSH);

    // 말 그리기 (✅ 뒤집기 적용)
    for (int i = 0; i < 32; i++)
    {
        chessPiece& p = app->pieces[i];
        if (!p.alive) continue;

        int drawX = FlipX(p.rect.left);
        int drawY = FlipY(p.rect.top);
        DrawImageGP(g_images[p.type], drawX, drawY, 50, 50);

        bool drawCheck = false;
        if (p.type == wking && (g_whiteInCheck || g_whiteInCheckmate))
        {
            SelectPenObject(g_whiteInCheckmate ? RGB(200, 0, 0) : RGB(255, 100, 100), 5, PS_SOLID);
            drawCheck = true;
        }
        if (p.type == king && (g_blackInCheck || g_blackInCheckmate))
        {
            SelectPenObject(g_blackInCheckmate ? RGB(200, 0, 0) : RGB(255, 100, 100), 5, PS_SOLID);
            drawCheck = true;
        }
        if (drawCheck || p.selected)
        {
            if (!drawCheck) SelectPenObject(RGB(0, 180, 255), 5, PS_SOLID);
            SelectBrushObject(NULL_BRUSH);
            int l = drawX - 25;
            int t = drawY - 25;
            int r = l + 100;
            int b = t + 100;
            Line(l, t, r, t); Line(l, b, r, b);
            Line(l, t, l, b); Line(r, t, r, b);
            SelectPenObject(NULL_PEN);
        }
    }

    // 게임 종료 UI
    if (g_gameOver)
    {
        const char* msg = "";
        if (g_winner == 1)      msg = "WHITE WINS!";
        else if (g_winner == 0) msg = "BLACK WINS!";
        else                    msg = "STALEMATE - DRAW";
        SelectFontObject("Malgun Gothic", 32, 1);
        SetTextColor(RGB(255, 220, 50));
        TextOut(g_resetBtn.left, g_resetBtn.top - 60, msg);
        SelectBrushObject(RGB(120, 120, 120));
        SelectPenObject(RGB(0, 0, 0), 2, PS_SOLID);
        Rectangle(g_resetBtn.left, g_resetBtn.top, g_resetBtn.right, g_resetBtn.bottom);
        SelectBrushObject(NULL_BRUSH); SelectPenObject(NULL_PEN);
        SelectFontObject("Malgun Gothic", 16, 1);
        SetTextColor(RGB(255, 255, 255));
        TextOut(g_resetBtn.left + 20, g_resetBtn.top + 14, "다시 시작하기");
    }

    // 이동 로그 출력
    SelectFontObject("Malgun Gothic", 13, 0);
    SetTextColor(RGB(0, 0, 0));
    for (int row = 0; row < 14; row++)
    {
        int i = g_moveLogCount - 1 - row;
        if (i < 0) break;
        MoveLog& m = g_moveLog[i];
        char left[32], right[16];
        int n = i + 1;
        left[0] = '0' + (n / 10) % 10; left[1] = '0' + (n % 10); left[2] = ' ';
        const char* cl = m.white ? "W " : "B ";
        left[3] = cl[0]; left[4] = cl[1]; left[5] = 0;
        right[0] = m.from[0]; right[1] = m.from[1];
        right[2] = m.capture ? 'x' : '-'; right[3] = '>';
        right[4] = m.to[0]; right[5] = m.to[1]; right[6] = 0;
        TextOut(106, 116 + row * 50, left);
        TextOut(206, 116 + row * 50, right);
    }

    // SET 버튼
    SelectBrushObject(RGB(220, 220, 220));
    SelectPenObject(RGB(0, 0, 0), 2, PS_SOLID);
    Rectangle(g_btnSettings.left, g_btnSettings.top, g_btnSettings.right, g_btnSettings.bottom);
    SelectBrushObject(NULL_BRUSH); SelectPenObject(NULL_PEN);
    SelectFontObject("Malgun Gothic", 16, 1);
    SetTextColor(RGB(0, 0, 0));
    TextOut(g_btnSettings.left + 30, g_btnSettings.top + 12, "SET");

    SelectFontObject("Malgun Gothic", 16, 0);
    SetTextColor(RGB(0, 0, 0));
    ShowDisplay();
}

// ===================================================
// 게임 리셋
// ===================================================
void ResetGame(AppData* app)
{
    g_moveLogCount = 0;
    g_gameOver = false;
    g_winner = -1;

    if (g_difficulty == DIF_PVP || g_difficulty == DIF_ONLINE_PVP)
        g_whiteTurn = (RandInt(2) == 0);
    else
        g_whiteTurn = true;

    g_whiteInCheck = false;
    g_blackInCheck = false;
    g_whiteInCheckmate = false;
    g_blackInCheckmate = false;
    g_stalemate = false;

    g_whiteAiPlayedThisTurn = false;
    g_promotionActive = false;
    g_promoPawnIdx = -1;
    g_promoWhite = false;
    g_showWhiteThink = false;

    g_enPassantX = -1;
    g_enPassantY = -1;
    g_aiDelay = 0;
    g_positionCount = 0;
    memset(g_positionHistory, 0, sizeof(g_positionHistory));

    // ✅ 오프라인 PVP - 랜덤 선공에 따라 초기 뒤집기 설정
    if (g_difficulty == DIF_PVP)
        g_boardFlipped = !g_whiteTurn;  // 검은색 선공이면 뒤집기
    else if (g_difficulty != DIF_ONLINE_PVP)
        g_boardFlipped = false;

    InitChessPieces(app);
    ClearMoveHints(app);
    for (int i = 0; i < 32; i++) app->pieces[i].selected = false;
    UpdateCheckState(app);
    OnDraw(app);
}

// ===================================================
// ESC 키 처리 타이머
// ===================================================
VOID CALLBACK InputTimerProc(HWND, UINT, UINT_PTR, DWORD)
{
    AppData* app = (AppData*)GetAppData();
    if (!app) return;
    bool escDown = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
    if (escDown && !g_prevEscDown && !g_promotionActive)
    {
        if (g_scene == SCENE_GAME)
        {
            for (int i = 0; i < 32; i++) app->pieces[i].selected = false;
            ClearMoveHints(app);
            g_scene = SCENE_SETTINGS;
            OnDraw(app);
        }
        else if (g_scene == SCENE_SETTINGS)
        {
            g_scene = SCENE_GAME;
            OnDraw(app);
        }
    }
    g_prevEscDown = escDown;

    if (g_difficulty == DIF_PVP || g_difficulty == DIF_ONLINE_PVP) return;

    if (g_scene == SCENE_GAME && !g_gameOver && !g_promotionActive &&
        !g_whiteTurn && !g_whiteAiPlayedThisTurn)
    {
        if (g_aiDelay == 0)
        {
            g_aiDelay = GetTickCount() + 500;
            g_showWhiteThink = true;
            OnDraw(app);
            return;
        }
        if (GetTickCount() >= g_aiDelay)
        {
            g_aiDelay = 0;
            g_showWhiteThink = false;
            AutoMoveBlack(app);
            UpdateCheckState(app);
            OnDraw(app);
        }
    }
}

// ===================================================
// 마우스 입력
// ===================================================
void OnMouseLeftDown(int, POINT pos)
{
    AppData* app = (AppData*)GetAppData();

    if (g_scene == SCENE_TITLE)
    {
        if (PtInRect(&g_btnStart, pos)) { g_scene = SCENE_DIFFICULTY; OnDraw(app); }
        else if (PtInRect(&g_btnExit, pos)) PostQuitMessage(0);
        return;
    }

    if (g_scene == SCENE_DIFFICULTY)
    {
        if (PtInRect(&g_btnEasy, pos)) g_difficulty = DIF_EASY;
        else if (PtInRect(&g_btnNormal, pos)) g_difficulty = DIF_NORMAL;
        else if (PtInRect(&g_btnHard, pos)) g_difficulty = DIF_HARD;
        else if (PtInRect(&g_btnHardcore, pos)) g_difficulty = DIF_HARDCORE;
        else if (PtInRect(&g_btnLocalPVP, pos))
        {
            g_difficulty = DIF_PVP;
            ResetGame(app);
            g_scene = SCENE_GAME;
            OnDraw(app);
            return;
        }
        else if (PtInRect(&g_btnOnlinePVP, pos))
        {
            g_difficulty = DIF_ONLINE_PVP;
            g_scene = SCENE_LOBBY;
            OnDraw(app);
            return;
        }
        else if (PtInRect(&g_btnBack, pos)) { g_scene = SCENE_TITLE; OnDraw(app); return; }
        else return;
        ResetGame(app);
        g_scene = SCENE_GAME;
        OnDraw(app);
        return;
    }

    if (g_scene == SCENE_LOBBY)
    {
        if (PtInRect(&g_btnHost, pos))
        {
            g_isHost = true;
            StartServer();
            g_scene = SCENE_WAITING;
            OnDraw(app);
        }
        else if (PtInRect(&g_btnJoin, pos))
        {
            g_isHost = false;
            if (!g_editIP)
                g_editIP = CreateEdit(820, 700, 280, 50, 1001, ES_AUTOHSCROLL);
            g_scene = SCENE_JOIN;
            OnDraw(app);
        }
        else if (PtInRect(&g_btnLobbyBack, pos))
        {
            g_scene = SCENE_DIFFICULTY;
            OnDraw(app);
        }
        return;
    }

    if (g_scene == SCENE_WAITING)
    {
        if (PtInRect(&g_btnWaitingBack, pos))
        {
            if (g_serverSocket) { DeleteServerSocket(g_serverSocket); g_serverSocket = NULL; }
            g_scene = SCENE_LOBBY;
            OnDraw(app);
        }
        return;
    }

    if (g_scene == SCENE_SETTINGS)
    {
        if (PtInRect(&g_btnSetBack, pos)) { g_scene = SCENE_GAME;       OnDraw(app); }
        else if (PtInRect(&g_btnSetReselect, pos)) { g_scene = SCENE_DIFFICULTY; OnDraw(app); }
        else if (PtInRect(&g_btnSetRestart, pos)) { g_scene = SCENE_GAME;       ResetGame(app); }
        return;
    }

    if (g_scene == SCENE_JOIN)
    {
        if (PtInRect(&g_btnConnect, pos))
        {
            GetCtrlName(g_editIP, g_ipBuffer, 16);
            JoinServer(g_ipBuffer);
        }
        else if (PtInRect(&g_btnJoinBack, pos))
        {
            if (g_editIP) { DestroyControl(g_editIP); g_editIP = NULL; }
            g_scene = SCENE_LOBBY;
            OnDraw(app);
        }
        return;
    }

    // 게임 화면
    if (PtInRect(&g_btnSettings, pos))
    {
        for (int i = 0; i < 32; i++) app->pieces[i].selected = false;
        ClearMoveHints(app);
        g_scene = SCENE_SETTINGS;
        OnDraw(app);
        return;
    }

    if (g_gameOver)
    {
        if (PtInRect(&g_resetBtn, pos)) ResetGame(app);
        return;
    }

    if (g_promotionActive)
    {
        for (int i = 0; i < 4; i++)
        {
            if (!PtInRect(&g_promoBtns[i], pos)) continue;
            ApplyPromotionChoice(app, i);
            g_whiteTurn = !g_whiteTurn;
            if (!g_whiteTurn) g_whiteAiPlayedThisTurn = false;
            // ✅ 오프라인 PVP - 프로모션 후 뒤집기
            if (g_difficulty == DIF_PVP) g_boardFlipped = !g_whiteTurn;
            UpdateCheckState(app);
            OnDraw(app);
            return;
        }
        return;
    }

    if (!g_whiteTurn && g_difficulty != DIF_PVP && g_difficulty != DIF_ONLINE_PVP) return;

    const int BL = 625, BT = 175, BR = 1425, BB = 975;
    bool inBoard = (pos.x >= BL && pos.x < BR && pos.y >= BT && pos.y < BB);

    // ✅ 뒤집기 적용 - 클릭 좌표를 실제 체스 좌표로 변환
    int clickX = g_boardFlipped ? (625 + 1325 - (BL + ((pos.x - BL) / 100) * 100)) : (BL + ((pos.x - BL) / 100) * 100);
    int clickY = g_boardFlipped ? (175 + 875 - (BT + ((pos.y - BT) / 100) * 100)) : (BT + ((pos.y - BT) / 100) * 100);
    int gx = clickX;
    int gy = clickY;

    int selIdx = GetSelectedPieceIndex(app);

    if (selIdx != -1)
    {
        if (!inBoard)
        {
            app->pieces[selIdx].selected = false;
            ClearMoveHints(app); OnDraw(app); return;
        }

        chessPiece& sel = app->pieces[selIdx];

        // 같은 편 다른 말 클릭 → 전환
        for (int i = 0; i < 32; i++)
        {
            if (i == selIdx || !app->pieces[i].alive) continue;
            int drawX = FlipX(app->pieces[i].rect.left);
            int drawY = FlipY(app->pieces[i].rect.top);
            RECT drawRect = { drawX, drawY, drawX + 100, drawY + 100 };
            if (!PtInRect(&drawRect, pos)) continue;
            if (IsWhite(app->pieces[i].type) != IsWhite(sel.type)) continue;
            if ((g_difficulty == DIF_PVP || g_difficulty == DIF_ONLINE_PVP) &&
                IsWhite(app->pieces[i].type) != g_whiteTurn) continue;
            sel.selected = false; ClearMoveHints(app);
            app->pieces[i].selected = true; CalcMoveHints(app, i);
            OnDraw(app); return;
        }

        // 같은 말 다시 클릭 → 해제
        {
            int drawX = FlipX(sel.rect.left);
            int drawY = FlipY(sel.rect.top);
            RECT drawRect = { drawX, drawY, drawX + 100, drawY + 100 };
            if (PtInRect(&drawRect, pos))
            {
                sel.selected = false; ClearMoveHints(app); OnDraw(app); return;
            }
        }

        // 이동 시도
        if (!CanMovePiece(app, selIdx, gx, gy)) return;
        if (!CanMoveWhenInCheck(app, selIdx, gx, gy)) return;

        int target = GetPieceAt(app, gx, gy);
        if (target != -1) { if (!CapturePiece(app, selIdx, target)) return; }
        else               MovePiece(app, selIdx, gx, gy);

        UpdateCheckState(app);
        app->pieces[selIdx].selected = false;
        ClearMoveHints(app);

        if (!g_whiteTurn && !g_gameOver && !g_promotionActive)
            g_showWhiteThink = true;

        OnDraw(app);
        return;
    }

    // 말 선택
    if (!inBoard) return;
    for (int i = 0; i < 32; i++)
    {
        chessPiece& p = app->pieces[i];
        if (!p.alive) continue;
        int drawX = FlipX(p.rect.left);
        int drawY = FlipY(p.rect.top);
        RECT drawRect = { drawX, drawY, drawX + 100, drawY + 100 };
        if (!PtInRect(&drawRect, pos)) continue;

        if (g_difficulty == DIF_PVP || g_difficulty == DIF_ONLINE_PVP)
        {
            if (IsWhite(p.type) != g_whiteTurn) return;
        }
        else
        {
            if (!IsWhite(p.type)) return;
        }

        for (int j = 0; j < 32; j++) app->pieces[j].selected = false;
        p.selected = true;
        ClearMoveHints(app); CalcMoveHints(app, i);
        OnDraw(app); return;
    }
}

MOUSE_MESSAGE(OnMouseLeftDown, NULL, NULL)

// ===================================================
// 메인
// ===================================================
int main()
{
    ChangeWorkSize(1920, 1080);
    SeedRng();

    AppData* app = (AppData*)CreateAppData(sizeof(AppData));
    SetupPromotionUI();
    LoadImages();
    InitChessPieces(app);
    ClearMoveHints(app);
    UpdateCheckState(app);

    SetTimer(NULL, 1, 16, InputTimerProc);

    g_scene = SCENE_TITLE;
    OnDraw(app);
    return 0;
}
