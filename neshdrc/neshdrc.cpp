#include "stdafx.h"
#include "7zip.h"

void SetConsoleSize()
{
	RECT rc;
	HWND hWnd = GetConsoleWindow();
	ShowWindow(hWnd, SW_HIDE);
	GetWindowRect(hWnd, &rc);
	system("mode con cols=250 lines=1000");
	SetWindowPos(hWnd,0,0,0,rc.right-rc.left,rc.bottom-rc.top,SWP_NOMOVE|SWP_NOZORDER);
	ShowWindow(hWnd, SW_SHOWNORMAL);
}

void ShowHelp()
{
	const char* msg = 
		"���ڿ���̨��NES�ļ�ͷ�鿴��\n\n"
		"����:\n"
		"\t����: ����Ů�� <QQ:191035066>\n"
		"\tʱ��: " __DATE__ " " __TIME__ "\n"
		"\t��ַ: https://github.com/movsb/neshdrc.git\n\n"
		"�����б�:\n"
		"\tR <�ļ����ļ���>	��ȡ�ļ����ļ���\n"
		"\tL			�г������Ѷ�ȡ��nes�ļ�ͷ��Ϣ\n"
		"\tM <Mapper��>		�г�����mapperΪ<mapper��>���ļ�\n"
		"\tQ			�˳�����\n"
		;
	puts(msg);
}

class CNesHeader
{
public:
	CNesHeader(CLibRef7Zip& lib7z)
		: m_lib7z(lib7z)
		, m_offsetdir(0)
	{}

private:
	void SkipWS(const char** pp)
	{
		const char* p = *pp;
		while(*p && (*p == ' ' || *p=='\t' || *p=='\n'))
			p++;
		*pp = p;
	}
	bool ReadString(std::string* ps, const char** pp)
	{
		std::string& s = *ps;
		const char* p = *pp;
		bool quote=false;

		s = "";
		while(*p){
			if(*p == '"'){
				if(quote){
					quote = false;
					*pp = ++p;
					return true;
				}
				else{
					quote = true;
					++p;
					continue;
				}
			}
			else if(!*p){
				if(quote){
					*pp = p;
					return false;
				}
				else{
					*pp = p;
					return true;
				}
			}
			else if(*p==' ' || *p=='\t'){
				if(quote){
					s += *p;
					++p;
					continue;
				}
				else{
					*pp = p;
					return true;
				}
			}
			else{
				s += *p;
				++p;
			}
		}
		*pp = p;
		
		return !quote;
	}

public:
	bool GetArgs(const char* cmd)
	{
		const char* p = cmd;
		std::string arg;

		m_args.clear();

		for(;;){
			SkipWS(&p);
			if(!*p) break;
			if(!ReadString(&arg, &p))
				return false;
			else{
				if(arg.size())
					m_args.push_back(arg);
			}
		}

// 		for(auto& x : m_args){
// 			std::cout << "[" << x << "]" << std::endl;
// 		}

		return true;
	}
	bool IsQuitArg()
	{
		return m_args.size() && (m_args[0][0]=='q' || m_args[0][0]=='Q');
	}
	void ShowInfo(CNesFile* f)
	{
		printf("Mapper: %d\tPROM: %3d\tVROM:%3d\t%s\n",
			f->hdr[7]&0xFF | f->hdr[6]>>4,
			f->hdr[4],
			f->hdr[5],
			f->fname.c_str()+m_offsetdir);
	}
	void DoCmd()
	{
		if(!m_args.size()) return;

		if(_stricmp(m_args[0].c_str(), "R") == 0){
			if(m_args.size() != 2){
				std::cout << "��������ȷ!" <<std::endl;
				return;
			}

			m_files.clear();

			auto attr = ::GetFileAttributes(m_args[1].c_str());
			if(attr == INVALID_FILE_ATTRIBUTES){
				std::cout << "�޷���ȡ�ļ���Ϣ, �����ļ�������?" << std::endl;
				return;
			}
			if(attr & FILE_ATTRIBUTE_DIRECTORY){
				std::vector<std::string> fs;
				m_offsetdir = strlen(m_args[1].c_str());
				GetDirectoryFiles(m_args[1].c_str(), &fs);
				for(auto& f : fs){
					GetArchiveHeaders(m_lib7z, &m_files, f.c_str());
				}
			}
			else{
				auto pbs = strrchr(m_args[1].c_str(),'\\');
				if(!pbs) pbs = strrchr(m_args[1].c_str(),'/');
				if(pbs){
					m_offsetdir = pbs-m_args[1].c_str()+1;
				}
				else{
					m_offsetdir = 0;
				}
				GetArchiveHeaders(m_lib7z, &m_files, m_args[1].c_str());
			}

			std::cout << "���� " << m_files.size() <<  " ���ļ�!" << std::endl;
			return;
		}
		else if(_stricmp(m_args[0].c_str(), "L") == 0){
			if(!m_files.size()){
				std::cout << "û���ļ�!" << std::endl;
				return;
			}

			for(auto& f : m_files){
				ShowInfo(&f);
			}
			return;
		}
		else if(_stricmp(m_args[0].c_str(), "M") == 0){
			if(m_args.size() != 2){
				std::cout << "��������ȷ!" <<std::endl;
				return;
			}

			int mapper = atoi(m_args[1].c_str());
			int count = 0;
			for(auto& f : m_files){
				if( (f.hdr[7]&0xFF | f.hdr[6]>>4) == mapper){
					count++;
					ShowInfo(&f);
				}
			}
			if(count == 0){
				std::cout << "�޴�mapper�ŵ��ļ�!" << std::endl;
			}
			return;
		}
		else{
			std::cout << "δ֪����!" << std::endl;
			return;
		}
	}

private:
	std::vector<std::string> m_args;
	std::vector<CNesFile> m_files;
	CLibRef7Zip& m_lib7z;
	int m_offsetdir;
};

int main()
{
	SetConsoleSize();

	ShowHelp();

	CLibRef7Zip lib7z("7z.dll");
	if(!lib7z.IsOK()){
		std::cout << "�޷�����7z.dll(������Ч��7z.dll)!" << std::endl;
		system("pause");
		return 1;
	}

	CNesHeader hdr(lib7z);
	char cmd[2048];
	for(;;){
		printf("\n����: ");
		fgets(cmd, sizeof(cmd), stdin);
		auto pn = strrchr(cmd, '\n');
		if(pn) *pn = '\0';
		if(!hdr.GetArgs(cmd)){
			printf("�����ʽ����ȷ!\n\n");
			continue;
		}

		if(hdr.IsQuitArg())
			break;
		
		hdr.DoCmd();
	}

	return 0;
}

