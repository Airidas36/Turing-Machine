#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <thread>
#include <fstream>
#include "ConsoleColor.h"
#include <windows.h>
#include <mutex>
#include <conio.h> 
using namespace std;
mutex mtx;

void gotoxy(int x, int y); // Funckija, keisti padeti konsoleje
void ShowConsoleCursor(bool showFlag);

class Turing
{
private:
	int head; // Galvutes pozicija
	string tape; // Duomenu juosta
	string state; // Busena
	string HaltingState;
	bool error, head_error; // Kintamasis, nustatyti ar ivyko klaida
	int steps; // Zingsniu skaitiklis
	int fix; // Kintamasis, atitinkamai atitraukiantis rodykles pozicija konsoleje, kad vizualiai graziai butu isspausdintos duomenu juostos
	typedef vector<tuple<string, string, string, string, string>> rules; // Vektorius, kurio i-tasis elementas talpina 5 reiskmes, <busena, simbolis, naujas simbolis, kryptis, nauja busena>
	rules R;
public:
	void operator ()() { Work(); }
	Turing() { head = 0, state = "0", error = false, head_error = false, steps = 0; } // Konstruktorius nustato pradines reiksmes
	void ReadFile(string file, int x)
	{
		ifstream input(file); //Atsidaro failas, failo pavadinimas perduodamas kaip argumentas is main'o
		input >> head; // Nuskaitoma galvutes pozicija
		input >> tape; // Nuskaitoma duomenu juosta
		fix = x; // Kintamasis x - Tiuringo masinu kiekis, atitinkamai kiek bus masinu, konsoleje rodyke bus nukeliama zemiau, atspaudindinti kitos masinos juosta
		string busena, nauja_busena, simbolis, naujas_simbolis, pozicija; // Sukuriami kintamieji, kuriuos nuskaite push'insim i vektoriu
		while (true) // Begalinis ciklas
		{
			input >> busena >> simbolis >> naujas_simbolis >> pozicija >> nauja_busena; // Nuskaitoma taiskyle
			if (!input) break; // jeigu nuskaitaityti nepavyko, reiskiasi pasiekeme failo pabaiga, ciklas nutraukiamas
			R.push_back(tuple<string, string, string, string, string>(busena, simbolis, naujas_simbolis, pozicija, nauja_busena));
		}
		input.close(); // Uzdaromas failas
	}
	void HaltState() // Algoritmas randantis masinos stabdymo busena
	{
		bool found;
		string halt;
		for (rules::const_iterator i = R.begin(); i != R.end(); i++)
		{
			halt = get<4>(*i);
			found = false;
			for (rules::const_iterator j = R.begin(); j != R.end(); j++)
			{
				if (halt == get<0>(*j)) 
					found = true;
			}
			if (!found)
			{
				HaltingState = halt; // Jeigu randama tokia busena, kuri nera aprasyta taisyklese, vadinasi, tai stabdymo busena
				break;
			}
		}
	}
	void Work()
	{
		HaltState();
		while (Halt(state)) // Ciklas, vykstantis, kol busena nelygi stabdymo busenai
		{
			mtx.lock(); // Uzrakiname gija
			if (_kbhit()) // Jeigu bus nuspaustas klavisas, sustabdysime Tiuringo masina
			{
				mtx.unlock();
				break;
			}
			gotoxy(0, 0 + fix * 4); // Atitinkamai pagal masinu kieki rodykle konsoleje paslenkama i apacia
			Rules(head); // Veikimo principas, argumentas - galvutes pozicija
			if (error || head_error) // Patikrina ar neivyko klaidu, ar buvo rastas busena atitinkantis simbolis, ar galvute neuzejo uz juostos ribu
			{
				if (head_error) PrintError("head", " "); // Jei ivyko klaida, ar galvute uzejo is juostos, stabdom cikla
				mtx.unlock(); // Pries sustabdant masina, atrakinam gija
				break;
			}
			steps++; // Skaiciuojami zingsniai
			cout << blue << "STEPS: " << steps << "  HEAD: " << head << "  STATE: " << state << " " << white << endl; // Atspausdini zingsniai, galvute pozicija, busena
			Tape(); // Atspausdinama duomenu juosta
			if (!Halt(state)) // Pasiziurima ar nereikia stabdyti
			{
				gotoxy(0, 0 + fix * 4 + 2);
				cout << green << "SUCCESSFULLY HALTED AFTER " << steps << " STEPS" << white;
			}
			mtx.unlock();
			Sleep(50);
		}
	}
	void Rules(int& head)
	{
		bool found = false; // Nustatomas found kintamasis, jeigu praejus ciklui arba ciklo eigoje jis netaps true, reiskia nebuvo rastas toks simbolis pagal dabartine busena
		string o_state, o_symbol, n_symbol, direction;
		if (head > 0 || head > tape.length())
		{
			o_symbol.push_back(tape[head - 1]); // Issaugomas simbolis, kuri lyginsim su taiskyklemis
			for (rules::const_iterator i = R.begin(); i != R.end(); ++i) // Ciklas, begantis per taisykles
			{
				if (o_symbol == get<1>(*i) && state == get<0>(*i)) // Jeigu simbolis lygus i-sios taisykles simboliui ir dabartine busena atitinka i-sios taisykles busenai, reiskiasi radom atitinkama taisykle
				{
					found = true; // Rasta taisykle
					n_symbol = get<2>(*i); // Nustatomas, koks bus naujas simbolis
					tape[head - 1] = n_symbol[0]; // Duomenu juostoje, galvutes pozicijoje esantis simbolis, keiciamas nauju
					direction = get<3>(*i); // Nustatoma kryptis
					if (direction == "L")	head--; // jeigu kryptis i kaire, galvutes pozicija slenkama i kaire, t.y galvutes skaicius mazinamas
					else if (direction == "R")	head++; // jeigu kryptis i desine, galvutes pozicija slenkama i desine, t.y galvutes skaicius didinamas
					state = get<4>(*i); // Nustatoma nauja busena
					break; // Sustabdomas ciklas
				}
			}
		}
		else head_error = true;
		if (!found) // jei atitinkama taisykle nerasta, error ivyko, nustatoma i true, atspaudinama klaida
		{
			error = true;
			PrintError(state, o_symbol);
		}
	}
	void PrintError(string abc, string symbol) // Klaidu pranesimai
	{
		gotoxy(0, 0 + fix * 4 + 2);
		if (abc == "state") cout << red << "HALTED. NO RULE FOR STATE " << blue << state << red << " AND SYMBOL " << blue << symbol << white; // Isvedama, jeigu ivyko klaida su busena, pranesama kokioje busenoje su kokiu simboliu si klaida ivyko
		else if (abc == "head") cout << red << "HEAD IS OUT OF TAPE BOUNDS, CHECK RULES FOR HUMAN ERRORS :" << white; // Pranesama kai galvute isejo is uz juostos ribu
		else cout << red << "UNDEFINED ERROR : " << white; // Jei nezinoma klaida
	}
	void Tape()
	{
		for (int i = 0; i < tape.length(); i++)
		{
			if (i == head - 1)	cout << red << "(" << tape[i] << ")" << white; // Atspausdinama duomenu juosta, duomuo, ant kurio yra galvute, apskliaudziamas ir nuspalvinamas
			else cout << tape[i];
		}
	}
	bool Halt(string state)
	{
		if (state == HaltingState) return false;
		else return true;
	}
	~Turing() {}
};

void PrintMenu()
{
	cout << blue << "Welcome to the menu, please choose a method to launch Turings Machine: \n";
	cout << "1. Launch a single Turing machine\n";
	cout << "2. Launch multiple Turing machines working synchronically\n";
	cout << "3. Exit program\n";
	cout << red << "INFO : To halt all machines at once, press any key.\n" << blue;
}

int main()
{
	int n = 0;
	char choice;
	while (true)
	{
		while (true)
		{
			system("CLS");
			ShowConsoleCursor(true);
			PrintMenu(); // Atspausdinami pasirinkimo variantai
			cout << red;
			cin >> choice; // Ivedamas pasirinkimas
			cout << white;
			if (choice == '1' || choice == '2' || choice == '3')	break; // Jeigu toks pasirinkimas egzistuoja einame toliau
			else
			{
				cout << red << "Pasirinktas neegzistuojantis variantas.\n" << blue; // Jeigu tokio nera, prasome vesti is naujo
				Sleep(1000);
			}
		}
		if (choice == '1')
		{
			n = 1;
			ShowConsoleCursor(false); // Isjungiamas rodykles mirksejimas konsole, vizualiai graziau atrodo
			Turing Machine; // Sukuriamas objektas
			string filename;
			system("CLS");
			cout << blue << "Iveskite failo pavadinima: " << red;
			cin >> filename; // Ivedamas failo pavadinimas
			Machine.ReadFile(filename, 0); // Nuskaitomas filas
			system("CLS");
			Machine.Work(); // Paleidziama masina
			cout << endl;
			system("pause");
			system("CLS");
		}
		else if (choice == '2')
		{
			system("CLS");
			cout << blue << "Iveskite kiek masinu paleisite: ";
			cin >> n; // Ivedamas masinu kiekis
			string* filenames;
			filenames = new string[n]; // Sukuriamas dinaminis failu pavadinimu masyvas
			for (int i = 0; i < n; i++)
			{
				cout << blue << "Iveskite visus failu vardus: \n";
				cin >> filenames[i]; // Ivedami failu, is kuriu skaitysim pavadinimai
			}
			system("CLS");
			ShowConsoleCursor(false);
			vector<thread> Threads(n); // Sukuriam giju vektoriu
			Turing* Machine = new Turing[n]; // Sukuriame dinamini objektu masyva
			for (int i = 0; i < n; i++)
				Machine[i].ReadFile(filenames[i], i); // Kiekvienam objektui kvieciama ReadFile funkcija, argumentai yra failu pavadinimo masyvo kintamieji

			for (int i = 0; i < n; i++)
				Threads[i] = thread(Machine[i]); // I giju masyva sudedame objektus

			for (int i = 0; i < n; i++)
				Threads[i].join();

			delete[]filenames; // Pasibaigus darbui, atlaisvinama atmintis 
			delete[]Machine;
			gotoxy(0, 0 + n * 4 );
			system("pause");
			system("CLS");
		}
		else break;
	}
	return 0;
}
void gotoxy(int x, int y)
{
	COORD pos = { x, y };
	HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorPosition(output, pos);
}
void ShowConsoleCursor(bool showFlag)
{
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_CURSOR_INFO     cursorInfo;

	GetConsoleCursorInfo(out, &cursorInfo);
	cursorInfo.bVisible = showFlag; // set the cursor visibility
	SetConsoleCursorInfo(out, &cursorInfo);
}
