typedef int BOOL;
#define TRUE 1
#define FALSE 0

#define PAGELINE	14
#define SPACE   	32
#define RETURN  	13
#define	ESC     	27
#define	UP      	72
#define DOWN    	80
#define LEFT		75
#define RIGHT		77
#define F1	        59
#define F2          60
#define F3          61
#define BS          14
#define HWINDOW      5
#define WWINDOW	    46
#define MAXLENGTH	20

void VideoMode();
void getlib();
void SetCursorSize(int, int);
void ScreenClear(int, int, int, int);
void WriteChar(int, int, int, int);
void WriteStr(int, int, char*, int);
void DrawLine(int, int, int, int, int);
int  FetchFile();
void SelectFilePointer(int);
void DisplayFile(int, int);
int  GetKey();
void End();
void FreeFile();
void FileOperator(int, int, int);
void ShowFileWindow();
void Copy();
void Rename();
void Delete();
void HideFileWindow(int, int);
void SetCursorPosition(int, int);
BOOL IsDir();
void ChangeDir();
void DisplayInfo();
void Start();
void ChangeName(char*);
void Delete();
void Confirm(int, int, int, int);