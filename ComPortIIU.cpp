#include <iostream>
#include <stdio.h>
#include <locale.h>
#define CE_SERIAL_IMPLEMENTATION
#include "D:/ComPortIIU/ceSerial.h"
#include <vector>
#include <iomanip>

using namespace std;

template <typename T>
ostream& operator<<(ostream& out, const vector<T>& data) {
	for (int i = 0; i < data.size(); i++) {
		out << hex << "0x" << static_cast<int>(data[i]) << ' ';
	}
	out << '\n';
	return out;
}


unsigned short CRC16_modbus(unsigned char* message, int len) {
	const unsigned short generator = 0xA001;
	unsigned short crc = 0xFFFF;
	for (int i = 0; i < len; ++i) {
		crc ^= (unsigned short)message[i];
		for (int b = 0; b < 8; ++b) {
			if ((crc & 1) != 0) {
				crc >>= 1;
				crc ^= generator;
			}
			else {
				crc >>= 1;
			}
		}
	}
	return crc;
}

template <typename T>
int checkCRC(const vector<T>& data) {
	if (data[1] >= 128) {
		cout << "Ошибка отправленного пакета. Код ошибки: " << hex << static_cast<int>(data[2]) << " - " 
			<< ( (data[2] == 1) ? ("Неизвестная функция") : 
				 (data[2] == 2) ? ("Неверная длина запроса") :
				 (data[2] == 3) ? ("Неверная контрольная сумма") : 
				 (data[2] == 4) ? ("Неверные данные запроса") :
				 (data[2] == 5) ? ("Память защищена") :
				 (data[2] == 6) ? ("Защита доступа для записи") :
				 (data[2] == 8) ? ("Защита доступа для чтения") :
				 (data[2] == 9) ? ("Занятность памяти") :
				 ("Неизвестная ошибка") ) ;
		cout << endl;
		return -1;
	}
	return 0;
}

int main() {
	setlocale(LC_ALL, "ru");

	ceSerial com("\\\\.\\COM2", 4800, 8, 'S', 1);

	cout << "Открытие порта " <<  com.GetPort().c_str() << endl;
	bool successFlag;
	bool lock = false;
	vector<unsigned char> buf;

	if (com.Open() == 0) {
		cout << "!Порт открыт\n";
	}
	else {
		cout << "!Ошибка открытия порта.\n";
		exit(0);
	}
	
	unsigned char Address[1] = { 0x4D };

	unsigned char readVersion[4] = { 0x00, 0x02 };
	unsigned short CRC = CRC16_modbus(readVersion, 2);
	readVersion[3] = CRC >> 8;
	readVersion[2] = CRC & 0xff;

	unsigned char readNumber[4] = { 0x01, 0x02 };
	CRC = CRC16_modbus(readNumber, 2);
	readNumber[3] = CRC >> 8;
	readNumber[2] = CRC & 0xff;
	
	bool flag = false;
	while (!flag) {
		cout << "Выберите действие: \n";
		cout << "1.Убедиться в адресе;\n2.Чтение версии ПО;\n3.Чтение номера счетчика;\n4.Выход\n";
		int option;
		cin >> option;
		switch (option) {
		case 1: {
			cout << "\tПосылаемый пакет: ";
			for (int i = 0; i < 1; i++) {
				cout << hex << "0x" << static_cast<int>(Address[i]) << ' ';
			}
			cout << '\n';
			com.Write(Address, 1);

			while (com.IsOpened()) {
				if (lock && !successFlag) {
					cout << '\n';
					lock = false;
					cout << "\tАдрес устройства: " << buf << endl;
					buf.clear();
					break;
				}
				char c = com.ReadChar(successFlag);
				if (successFlag) {
					buf.push_back(c);
					lock = true;
				}
			}
			break;
		}
		case 2: {
			cout << '\n' << "\tПосылаемый пакет: ";
			for (int i = 0; i < 4; i++) {
				cout << hex << "0x" << static_cast<int>(readVersion[i]) << ' ';
			}
			cout << '\n';
			com.Write(Address, 1);
			com.Write(readVersion, 4);

			while (com.IsOpened()) {
				if (lock && !successFlag) {
					cout << '\n';
					lock = false;
					if (checkCRC(buf) == -1) {
						break;
					}
					cout << "\tКод операции - " << dec << static_cast<int>(buf[1]) << " ";
					cout << "Счетчик байт - " << dec << static_cast<int>(buf[2]);
					cout << "\n\tВерсия ПО: номер сборки - ";
					cout << dec << static_cast<int>(buf[3]) << " ";
					cout << ", номер версии - ";
					cout << dec << static_cast<int>(buf[4]) << " ";
					cout << ", тип счетчика - ";
					for (int i = 5; i <= 7; i++) {
						cout << dec << static_cast<int>(buf[i]) << " ";
					}
					cout << endl << endl;
					buf.clear();
					break;
				}
				char c = com.ReadChar(successFlag);
				if (successFlag) {
					buf.push_back(c);
					lock = true;
				}
			}
			break;
		}
		case 3: {
			cout << "\n\tПосылаемый пакет: ";
			for (int i = 0; i < 4; i++) {
				cout << hex << "0x" << static_cast<int>(readNumber[i]) << ' ';
			}
			cout << '\n';
			com.Write(Address, 1);
			com.Write(readNumber, 4);

			while (com.IsOpened()) {
				if (lock && !successFlag) {
					cout << '\n';
					lock = false;
					if (checkCRC(buf) == -1) {
						break;
					}
					cout << "\tКод операции - " << dec << static_cast<int>(buf[1]) << " ";
					cout << "Счетчик байт - " << dec << static_cast<int>(buf[2]);
					cout << "\n\tНомер счетчика - ";
					for (int i = 3; i <= 5; i++) {
						cout << dec << static_cast<int>(buf[i]) << " ";
					}
					cout << endl << endl;
					buf.clear();
					break;
				}
				char c = com.ReadChar(successFlag);
				if (successFlag) {
					buf.push_back(c);
					lock = true;
				}
			}
			break;
		}

		case 4:
			flag = true;
			break;

		default:
			cout << "Введите правильное значение!\n";
		}
	}

	cout << "\n!Закрытие порта " << com.GetPort().c_str() << endl;
	com.Close();
	return 0;
}
