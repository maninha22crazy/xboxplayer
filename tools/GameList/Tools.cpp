#include "tools.h"
#include <algorithm>

unsigned int starttime = 0;

using namespace std;
using std::string;

void TokenizeA(const string& str,
                      vector<string>& tokens,
                      const string& delimiters)
{
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while ( (wstring::npos != pos && pos != 0xffffffff) || string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}

void TokenizeW(const wstring& str,
                      vector<wstring>& tokens,
                      const wstring& delimiters)
{
    // Skip delimiters at beginning.
    wstring::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    wstring::size_type pos     = str.find_first_of(delimiters, lastPos);

    while ( (wstring::npos != pos && pos != 0xffffffff) || string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}

struct lowercase_func {
	void operator()(std::string::value_type& v) { v = (char)tolower(v); }
};

string make_lowercaseA(string s) {
	for_each(s.begin(), s.end(), lowercase_func());
	return s;
}

struct uppercase_func {
	void operator()(std::string::value_type& v) { v = (char)toupper(v); }
};

string make_uppercaseA(string s) {
	for_each(s.begin(), s.end(), uppercase_func());
	return s;
}

struct lowercasew_func {
	void operator()(std::wstring::value_type& v) { v = (char)tolower(v); }
};

wstring make_lowercaseW(wstring s) {
	for_each(s.begin(), s.end(), lowercasew_func());
	return s;
}

struct uppercasew_func {
	void operator()(std::wstring::value_type& v) { v = (char)toupper(v); }
};

wstring make_uppercaseW(wstring s) {
	for_each(s.begin(), s.end(), uppercasew_func());
	return s;
}

wstring make_titlecase(wstring s)
{
	wstring res;
	res.reserve(s.size());

	wstring::iterator itr;
	char last = ' ';
	for (itr = s.begin() ; itr != s.end() ; itr++)
	{
		if (last == ' ' || last == '(' || last == '{' || last == '[')
		{
			res.push_back((wchar_t)toupper(*itr));
		} else {
			res.push_back((wchar_t)tolower(*itr));
		}

		last = (char)*itr;
	}

	return res;
}

wstring str_replaceW(wstring source, wstring find, wstring replace)
{
	int srcpos = source.find(find);
	if (srcpos < 0) return source;

	wstring res = source.replace(srcpos,find.size(),replace);

	return res;
}

wstring str_replaceallW(wstring source, wstring find, wstring replace)
{
	int srcpos = source.find(find);
	while (srcpos > -1)
	{
		source = source.replace(srcpos,find.size(),replace);
		srcpos = source.find(find);
	}

	return source;
}

string str_replaceA(string source, string find, string replace)
{
	int srcpos = source.find(find);
	if (srcpos < 0) return source;

	string res = source.replace(srcpos,find.size(),replace);

	return res;
}

string str_replaceallA(string source, string find, string replace)
{
	int srcpos = source.find(find);
	while (srcpos > -1)
	{
		source = source.replace(srcpos,find.size(),replace);
		srcpos = source.find(find);
	}

	return source;
}

string wstrtostr(wstring wstr)
{
	string s(wstr.begin(), wstr.end());
	s.assign(wstr.begin(), wstr.end());
	return s;
}

wstring strtowstr(string str)
{
	wstring s(str.begin(), str.end());
	s.assign(str.begin(), str.end());
	return s;
}

/*
bool DirectoryInfo(string directory, vector<DI_Item>& files, vector<DI_Item>& folders, bool clear)
{
	printf("Scanning Dir %s\n",directory.c_str());
	WIN32_FIND_DATA findFileData;
	memset(&findFileData,0,sizeof(WIN32_FIND_DATA));
	string searchcmd = directory+"\\*";
	HANDLE hFind = FindFirstFile(searchcmd.c_str(), &findFileData);
	if (hFind == INVALID_HANDLE_VALUE)
		return false;

	if (clear)
	{
		files.clear();
		folders.clear();
	}
	do {
		string s = findFileData.cFileName;
		if (s == ".") continue;
		if (s == "..") continue;

		DI_Item item;
		item.Path = directory;
		item.FileName = s;
		item.attribs = findFileData.dwFileAttributes;
		if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			folders.push_back(item);
		} else{
			files.push_back(item);
		}
	} while (FindNextFile(hFind, &findFileData));
	FindClose(hFind);

	return true;
}*/

wstring LastFolderW(wstring folder)
{
	if (folder.at(folder.size()-1) == L'\\')
		folder = folder.substr(0,folder.size()-1);
	folder = folder.substr(folder.rfind(L"\\")+1);

	return folder;
}

wstring FileExtW(wstring filename)
{
	int dotpos = (int)filename.rfind(L".");
	if (dotpos == 0) return L"";
	wstring lc = make_lowercaseW(filename.substr(dotpos+1));
	return lc;
}

wstring FileNoExtW(wstring filename)
{
	int dotpos = (int)filename.rfind(L".");
	if (dotpos == 0) return filename;
	return filename.substr(0,dotpos);
}

string LastFolderA(string folder)
{
	if (folder.at(folder.size()-1) == '\\')
		folder = folder.substr(0,folder.size()-1);
	folder = folder.substr(folder.rfind("\\")+1);

	return folder;
}

string FileExtA(string filename)
{
	int dotpos = (int)filename.rfind(".");
	if (dotpos == 0) return "";
	string lc = make_lowercaseA(filename.substr(dotpos+1));
	return lc;
}

string FileNoExtA(string filename)
{
	int dotpos = (int)filename.rfind(".");
	if (dotpos == 0) return filename;
	return filename.substr(0,dotpos);
}


bool isbadcharw(WCHAR in)
{
	if (in == L' ' || in == L'\t' || in == L'\r' || in == L'\n') return true;
	return false;
}

wstring TrimLeftW(wstring str) 
{
	if (str.size() == 0) return L"";
	wstring::iterator i;
	for (i = str.begin(); i != str.end(); i++) {
		if (!isbadcharw(*i)) {
			break;
		}
	}
	if (i == str.end()) {
		str.clear();
	} else {
		str.erase(str.begin(), i);
	}
	return str;
}

wstring TrimRightW(wstring str)
{
	if (str.size() == 0) return L"";
	wstring::iterator i;
	for (i = str.end() - 1; ;i--) {
		if (!isbadcharw(*i)) {
			str.erase(i + 1, str.end());
			break;
		}
		if (i == str.begin()) {
			str.clear();
			break;
		}
	}
	return str;
}

wstring TrimW(wstring s) 
{
	return TrimLeftW(TrimRightW(s));
}

bool isbadchara(WCHAR in)
{
	if (in == ' ' || in == '\t' || in == '\r' || in == '\n') return true;
	return false;
}

string TrimLeftA(string str) 
{
	if (str.size() == 0) return "";
	string::iterator i;
	for (i = str.begin(); i != str.end(); i++) {
		if (!isbadchara(*i)) {
			break;
		}
	}
	if (i == str.end()) {
		str.clear();
	} else {
		str.erase(str.begin(), i);
	}
	return str;
}

string TrimRightA(string str)
{
	if (str.size() == 0) return "";
	string::iterator i;
	for (i = str.end() - 1; ;i--) {
		if (!isbadchara(*i)) {
			str.erase(i + 1, str.end());
			break;
		}
		if (i == str.begin()) {
			str.clear();
			break;
		}
	}
	return str;
}

string TrimA(string s) 
{
	return TrimLeftA(TrimRightA(s));
}

wstring TrimRightStr(wstring str,wstring crop)
{
	if (str.size() < crop.size()) return str;
	wstring temp = str.substr(str.size()-crop.size(),crop.size());
	if (_wcsicmp(temp.c_str(),crop.c_str()) == 0)
	{
		return  str.substr(0,str.size()-crop.size());
	}	
	return str;
}

wstring sprintfaW(const WCHAR *format, ...)
{
	WCHAR temp[16384];

	va_list ap;
	va_start (ap, format);
	vswprintf_s (temp,16384, format, ap);
	va_end (ap);

	return temp;
}

string sprintfaA(const char *format, ...)
{
	char temp[16384];

	va_list ap;
	va_start (ap, format);
	vsprintf_s (temp,16384, format, ap);
	va_end (ap);

	return temp;
}

wstring mstohms(int time)
{
	int hours = 0;
	int mins = 0;
	int secs = 0;

	secs = time / 1000;
	mins = secs / 60;
	secs = secs % 60;

	hours = mins / 60;
	mins = mins % 60;

	if (hours > 0)
	{
		return sprintfa(L"%d:%02d:%02d",hours,mins,secs);
	} else {
		return sprintfa(L"%d:%02d",mins,secs);
	}
}

wstring stohms(int time)
{
	return mstohms(time * 1000);
}


int FileExistsA(string filename)
{
	return GetFileAttributes(filename.c_str()) != 0xFFFFFFFF;
}

int FileExistsW(wstring filename)
{
	return FileExistsA(wstrtostr(filename));
}

wstring escape(wstring in)
{
	wstring out = L"";

	for (unsigned int i = 0 ; i < in.size() ; i++)
	{
		if (in.at(i) == L'&')
			out += L"&amp;";
		else if (in.at(i) == L'>')
			out += L"&gt;";
		else if (in.at(i) == L'<')
			out += L"&lt;";
		else if (in.at(i) == L'\'')
			out += L"&apos;";
		else if (in.at(i) == L'\"')
			out += L"&quot;";
		else 
			out += in.at(i);
	}
	return out;
}

wstring unescape(wstring in)
{
	//if (in.find("&") == in.npos) 
		return in;

	/*string out = "";

	for (unsigned int i = 0 ; i < in.size() ; i++)
	{
		if (in.at(i) == '&')
		{
			string code;
			i++;

			while (in.at(i) != ';')
			{
				code += in.at(i);
				i++;
			}

			if (code.at(0) == '#') // numerical jobby
			{
				int charcode = atoi(code.substr(1).c_str());
				char temp[2];
				temp[0] = charcode;
				temp[1] = 0;
				out.append(temp);
			} else {
				_ASSERT(FALSE);
				_ASSERT(TRUE);
			}
		}
		else 
			out += in.at(i);
	}
	return out;*/
}


const float fast_atof_table[] =	{
										0.f,
										0.1f,
										0.01f,
										0.001f,
										0.0001f,
										0.00001f,
										0.000001f,
										0.0000001f,
										0.00000001f,
										0.000000001f,
										0.0000000001f,
										0.00000000001f,
										0.000000000001f,
										0.0000000000001f,
										0.00000000000001f,
										0.000000000000001f
									};

//! Provides a fast function for converting a string into a float,
//! about 6 times faster than atof in win32.
// If you find any bugs, please send them to me, niko (at) irrlicht3d.org.
WCHAR* fast_atof_move(WCHAR* c, float& out)
{
	bool inv = false;
	WCHAR *t;
	float f;

	if (*c==L'-')
	{
		c++;
		inv = true;
	}

	f = (float)wcstol(c, &t, 10);

	c = t;

	if (*c == L'.')
	{
		c++;

		float pl = (float)wcstol(c, &t, 10);
		pl *= fast_atof_table[t-c];

		f += pl;

		c = t;

		if (*c == L'e')
		{
			++c;
			float exp = (float)wcstol(c, &t, 10);
			f *= (float)pow(10.0f, exp);
			c = t;
		}
	}

	if (inv)
		f *= -1.0f;
	
	out = f;
	return c;
}

//! Provides a fast function for converting a string into a float,
//! about 6 times faster than atof in win32.
// If you find any bugs, please send them to me, niko (at) irrlicht3d.org.
const WCHAR* fast_atof_move_const(const WCHAR* c, float& out)
{
	bool inv = false;
	WCHAR *t;
	float f;

	if (*c==L'-')
	{
		c++;
		inv = true;
	}

	f = (float)wcstol(c, &t, 10);

	c = t;

	if (*c == L'.')
	{
		c++;

		float pl = (float)wcstol(c, &t, 10);
		pl *= fast_atof_table[t-c];

		f += pl;

		c = t;

		if (*c == L'e') 
		{ 
			++c; 
			float exp = (float)wcstol(c, &t, 10); 
			f *= (float)powf(10.0f, exp); 
			c = t; 
		}
	}

	if (inv)
		f *= -1.0f;
	
	out = f;
	return c;
}


float fast_atof(const WCHAR* c)
{
	float ret;
	fast_atof_move_const(c, ret);
	return ret;
}

void aGetFileSize(string filename, DWORD & modified, DWORD & high)
{
	HANDLE file = CreateFile(filename.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,NULL,NULL);
	modified = GetFileSize(file,&high);
	CloseHandle(file);
}

int GetFileCreated(wstring filename)
{
	struct __stat64 buf;
	int result;
	result = _wstat64( filename.c_str(), &buf );
	result = 0;
	if (result == 0)
	{
		return (int)buf.st_ctime;
	} else {
		// TODO: file not found, but generall this is something messed up, so try to get the filesize
		return 0;
	}
}

void FindDataToStats(WIN32_FIND_DATA* findFileData, int & size, int & modified)
{
	INT64 bsize = findFileData->nFileSizeHigh;
	bsize = bsize << 32;
	bsize += findFileData->nFileSizeLow;
	bsize = bsize / 1024;

	size = (int)bsize;

	
	INT64 bmodified = findFileData->ftLastWriteTime.dwHighDateTime;
	bmodified = bmodified << 32;
	bmodified += findFileData->ftLastWriteTime.dwLowDateTime;
	bmodified = bmodified / 10000000;
	bmodified -= 11644473600;

	modified = (int)bmodified;
}

wstring formatsize(float val, wstring type)
{
	if (val > 100)
	{
		return sprintfa(L"%0.0f %s",val,type.c_str());
	}

	return sprintfa(L"%0.1f %s",val,type.c_str());
}

/*string format_size(string in)
{
	int num = atoi(in.c_str());
	return format_size(num);
}

string format_size(int in)
{
	if (in > 1073741824) // Tb
	{
		float val = (float)in / 1073741824;
		return formatsize(val,"Tb");
	}
	if (in > 1048576) // Gb
	{
		float val = (float)in / 1048576;
		return formatsize(val,"Gb");
	}
	if (in > 1024) // Mb
	{
		float val = (float)in / 1024;
		return formatsize(val,"Mb");
	}

	return sprintfa("%d Kb",in);
}*/

bool IsFolder(string filename)
{
	WIN32_FIND_DATA findFileData;
	memset(&findFileData,0,sizeof(WIN32_FIND_DATA));
	HANDLE hFind = FindFirstFile(filename.c_str(), &findFileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
		return false;
	}

	if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		FindClose(hFind);
		return true;
	}

	FindClose(hFind);
	return false;
}

bool SortAlphabetical(const wstring left, const wstring right) 
{ 
	int res = _wcsicmp(left.c_str(),right.c_str());
	if (res < 0) return true;
	return false;
}


void outputcmd(vector<wstring> commandparts, vector<wstring>& result)
{
	result.push_back(L"<div class='h4'>Commands</div>");
	for (unsigned int i = 0 ; i < commandparts.size() ; i++)
	{
		result.push_back(commandparts.at(i) + L"<br>");
	}
}
/*
string ImageFromFolder(string path)
{
	path = make_lowercase(path);
	string * res = (string*)g_CoverCache->GetData(path);
	if (res != NULL)
		return *res;

	string temp = ImageFromFolderQuick(path);
	if (temp.size() > 0) 
		return temp;

	if (FileExists(path + "\\cover.jpg")) return "cover.jpg";
	if (FileExists(path + "\\front.jpg")) return "front.jpg";

	if (FileExists(path + "\\cd.jpg")) return "cd.jpg";
	if (FileExists(path + "\\cover.png")) return "cover.png";
	if (FileExists(path + "\\front.png")) return "front.png";
	if (FileExists(path + "\\cd.png")) return "cd.png";

	if (FileExists(path + "\\cover.gif")) return "cover.gif";
	if (FileExists(path + "\\front.gif")) return "front.gif";
	if (FileExists(path + "\\cd.gif")) return "cd.gif";

	WIN32_FIND_DATA findFileData;
	memset(&findFileData,0,sizeof(WIN32_FIND_DATA));
	string searchcmd = path+"\\*";
	HANDLE hFind = FindFirstFile(searchcmd.c_str(), &findFileData);

	string result = "";
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do {
			string s = findFileData.cFileName;
			if (s == ".") continue;
			if (s == "..") continue;
			if (FileExt(s) == "jpg")
			{
				result = s;
				break;
			}
			if (FileExt(s) == "png")
			{
				result = s;
				break;
			}
			if (FileExt(s) == "gif")
			{
				result = s;
				break;
			}
		} while (FindNextFile(hFind, &findFileData));
		FindClose(hFind);
	}

	string * str = new string(result);
	g_CoverCache->SetData(path, str);


	return result;
}

string ImageFromFolderQuick(string path)
{
	path = make_lowercase(path);
	string * res = (string*)g_CoverCache->GetData(path);
	if (res != NULL)
		return *res;

	if (FileExists(path + "\\folder.jpg")) 
	{
		string * str = new string("folder.jpg");
		g_CoverCache->SetData(path, str);
		return "folder.jpg";
	}

	if (FileExists(path + "\\folder.png"))
	{
		string * str = new string("folder.png");
		g_CoverCache->SetData(path, str);
		return "folder.jpg";
	}

	if (FileExists(path + "\\folder.gif"))
	{
		string * str = new string("folder.png");
		g_CoverCache->SetData(path, str);
		return "folder.jpg";
	}

	return "";
}*/

HWND FindClient()
{
/*#ifdef _DEBUG
	return NULL;
#endif*/
	//HWND hWnd = FindWindow(WINDOWCLASS,WINDOWCLASS);
	return 0;
}

//#define PLAYERUIOVERRIDE
/*
void MinimizeClient()
{
	HWND hWnd = FindClient();
	if (!hWnd) return;

#ifdef PLAYERUIOVERRIDE
	// override players UI
	SendMessage(hWnd, WM_USER+1,1,0);
#else
	// allow player to use its own ui
	SendMessage (hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
	//SendMessage(hWnd, WM_USER+1,1,0);
	//ShowWindow(hWnd, SW_HIDE);
#endif
}*/
/*
void RestoreClient()
{
	HWND hWnd = FindClient();
	if (!hWnd) return;

#ifdef PLAYERUIOVERRIDE
	// override players UI
	SendMessage(hWnd, WM_USER+1,2,0);
#else
	SendMessage (hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
	//SendMessage(hWnd, WM_USER+1,2,0);
	//ShowWindow(hWnd, SW_RESTORE);
	SetForegroundWindow(hWnd);
#endif
}*/

int FilesInDir(string path)
{
	WIN32_FIND_DATA findFileData;
	memset(&findFileData,0,sizeof(WIN32_FIND_DATA));
	string searchcmd = path+"\\*";
	HANDLE hFind = FindFirstFile(searchcmd.c_str(), &findFileData);
	if (hFind == INVALID_HANDLE_VALUE)
		return 0;

	int count = 0;

	do {
		string s = findFileData.cFileName;
		if (s == ".") continue;
		if (s == "..") continue;

		count++;
		
	} while (FindNextFile(hFind, &findFileData));
	FindClose(hFind);

	return count;
}

void FileToStringA(string & result, string filename)
{
	FILE * fp;
	if (fopen_s(&fp,filename.c_str(),"r") == 0)
	{
		fseek(fp,0,SEEK_END);
		int size = ftell(fp);
		fseek(fp,0,SEEK_SET);
		char * buffer = new char[size+1];
		memset(buffer,0,size+1);

		result = "";
		result.reserve(size+1);
		
		int read = fread(buffer,1,size,fp);
		buffer[read] = 0;
		result = buffer;
		fclose(fp);
		SAFE_DELETE_A( buffer );
	} else {
		result = "";
	}
}

void FileToStringW(wstring & result, wstring filename)
{
	FILE * fp;
	if (_wfopen_s(&fp,filename.c_str(),L"r") == 0)
	{
		fseek(fp,0,SEEK_END);
		int size = ftell(fp);
		fseek(fp,0,SEEK_SET);
		WCHAR * buffer = new WCHAR[size+1];
		memset(buffer,0,size+1);

		result = L"";
		result.reserve(size+1);
		
		int read = fread(buffer,1,size,fp);
		buffer[read] = 0;
		result = buffer;
		fclose(fp);
		SAFE_DELETE_A( buffer );
	} else {
		result = L"";
	}
}


void StringToFileA(string &data, string filename)
{
	FILE * fp;
	if (fopen_s(&fp,filename.c_str(),"w") == 0)
	{
		fwrite(data.c_str(),1,data.size(),fp);
		fclose(fp);
	}
}

void StringToFileW(wstring &data, wstring filename)
{
	FILE * fp;
	if (_wfopen_s(&fp,filename.c_str(),L"w") == 0)
	{
		fwrite(data.c_str(),1,data.size()*2,fp);
		fclose(fp);
	}
}


bool ExtInList(wstring ext, vector<wstring>* filetypes)
{
	if (!filetypes)
		return true;

	if (filetypes->size() == 0) return true;
	for (unsigned int i = 0 ; i < filetypes->size() ; i++)
	{
		if (filetypes->at(i) == ext) return true;
	}
	return false;
}

const char NumericMap[256] = 
{
    /*      0 1 2 3  4 5 6 7  8 9 A B  C D E F */
    /* 0 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 1 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 2 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 3 */ 0,1,2,3, 4,5,6,7, 8,9,0,0, 0,0,0,0,
    
    /* 4 */ 0,2,2,2, 3,3,3,4, 4,4,5,5, 5,6,6,6,
    /* 5 */ 7,7,7,7, 8,8,8,9, 9,9,9,0, 0,0,0,0,
    /* 4 */ 0,2,2,2, 3,3,3,4, 4,4,5,5, 5,6,6,6,
    /* 5 */ 7,7,7,7, 8,8,8,9, 9,9,9,0, 0,0,0,0,
    
    /* 8 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 9 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* A */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* B */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    
    /* C */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* D */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* E */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* F */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
};

const char NumericMap2[10] = {'0','1','2','3','4','5','6','7','8','9'};

string make_numeric(string in)
{
	string result;
	result.reserve(in.size());
	char res = 0;
	for (unsigned int i = 0 ; i < in.size() ; i++)
	{
		res = NumericMap[in.at(i)];
		if (res > 0)
			result.append(1,NumericMap2[res]);
	}

	return result;
}

const char noncharnummap[256] = 
{
    /*       0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F */
    /* 0 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 1 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 2 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 3 */ 1,1,2,3, 4,5,6,7, 8,9,0,0, 0,0,0,0,
    
    /* 4 */ 0,2,2,2, 3,3,3,4, 4,4,5,5, 5,6,6,6,
    /* 5 */ 7,7,7,7, 8,8,8,9, 9,9,9,0, 0,0,0,0,
    /* 4 */ 0,2,2,2, 3,3,3,4, 4,4,5,5, 5,6,6,6,
    /* 5 */ 7,7,7,7, 8,8,8,9, 9,9,9,0, 0,0,0,0,
    
    /* 8 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 9 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* A */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* B */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    
    /* C */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* D */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* E */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* F */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
};


wstring replace_noncharnum(wstring in)
{
	wstring result;
	result.reserve(in.size());
	char res = 0;
	for (unsigned int i = 0 ; i < in.size() ; i++)
	{
		res = noncharnummap[in.at(i)];
		if (res > 0)
			result.append(1,in.at(i));
	}

	return result;	
}

wstring replace_numtochar(wstring in)
{
	wstring result;
	result.reserve(in.size());

	for (unsigned int i = 0 ; i < in.size() ; i++)
	{
		char curchar = (char)in.at(i);
		switch (curchar)
		{
		case L'2':
			result.append(1,L'a');
			break;
		case L'3':
			result.append(1,L'd');
			break;
		case L'4':
			result.append(1,L'g');
			break;
		case L'5':
			result.append(1,L'j');
			break;
		case L'6':
			result.append(1,L'm');
			break;
		case L'7':
			result.append(1,L'p');
			break;
		case L'8':
			result.append(1,L't');
			break;
		case L'9':
			result.append(1,L'w');
			break;
		}
	}

	return result;	
}

wstring make_filter_label(wstring filter, wstring text)
{
	wstring result = L"";
	if (text.size() > 0)
	{
		text = replace_noncharnum(text);
		text = text.substr(0,filter.size());
		result = text;
	} else {
		result = replace_numtochar(filter);
	}

	result = make_uppercaseW(result);

	return result;
}

wstring make_filter_label_2(wstring filter)
{
	/*string result = "";
	if (text.size() > 0)
	{
		text = replace_noncharnum(text);
		text = text.substr(0,filter.size());
		result = text;
	} else {
		result = replace_numtochar(filter);
	}

	result = make_uppercase(result);*/

	return filter;
}

unsigned int MakeUID()
{
	unsigned int id;
	//rand_s(&id);
	return id;
}

wstring MakeBigUID()
{
	unsigned int id1;
	//rand_s(&id1);
	unsigned int id2;
	//rand_s(&id2);
	unsigned int id3;
	//rand_s(&id3);

	return sprintfa(L"%u-%u-%u",id1,id2,id3);
}

string DecodeHtmlA(string in)
{
	while (in.find("&") != in.npos)
	{
		string code = in.substr(in.find("&"));
		code = code.substr(0,code.find(";")+1);
		if (!code.empty())
		{
			if (code == "&amp;")
			{
				in = str_replaceA(in,code,"&");
			} else {
				in = str_replaceA(in,code,"");
			}
		} else {
			break;
		}
	}
	return in;
}

wstring DecodeHtmlW(wstring in)
{
	while (in.find(L"&") != in.npos)
	{
		wstring code = in.substr(in.find(L"&"));
		code = code.substr(0,code.find(L";")+1);
		if (!code.empty())
		{
			if (code == L"&amp;")
			{
				in = str_replace(in,code,L"&");
			} else {
				in = str_replace(in,code,L"");
			}
		} else {
			break;
		}
	}
	return in;
}

wstring DecodeHtmlW(string in)
{
	int neededbuffer = MultiByteToWideChar(CP_UTF8,0,in.c_str(),-1,NULL,0);
	if (neededbuffer == 0)
		return NULL;
	WCHAR* buf2 = new WCHAR[neededbuffer];
	memset(buf2,0,neededbuffer);
	int res = MultiByteToWideChar(CP_UTF8,0,in.c_str(),-1,buf2,neededbuffer);
	if (res == 0)
		return NULL;

	wstring result = DecodeHtmlW(buf2);
	delete buf2;
	return result;
}
