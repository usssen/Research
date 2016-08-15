#include"aging.h"
#include<math.h>
#include<fstream>
#include<iostream>

using namespace std;

double Rate[15][3];
double a, alpha, n;

void ReadAgingData(){
	fstream file;
	file.open("AgingRate.txt");
	for (int i = 1; i <=5; i++){
		file >> Rate[i][0] >> Rate[i][1] >> Rate[i][2];
	}
	file >> Rate[10][0] >> Rate[10][1] >> Rate[10][2];
	for (int i = 6; i < 10; i++)
		for (int j = 0; j < 3;j++)
		Rate[i][j] = (Rate[10][j] - Rate[5][j]) / 5 * (i - 5) + Rate[5][j];
	file.close();
	file.open("Parameter.txt");
	file >> a >> alpha >> n;
	file.close();
}

double AgingRate(AGINGTYPE status, double year){	
	
	double second = year * 365 * 86400;	//0.0039*(0.5*t)^0.2 + 1	average case -> worst case即去掉0.5(alpha)
	int y = year;	//轉整數
	if (year > 10){
		switch (status){
		case DCC_M:
		case DCC_NONE:
			return (Rate[10][1] - Rate[9][1]) *(year - 10) + Rate[10][1];
			break;
		case DCC_F:
			return (Rate[10][2] - Rate[9][2]) *(year - 10) + Rate[10][2];
		case DCC_S:
			return (Rate[10][0] - Rate[9][0]) *(year - 10) + Rate[10][0];
		case FF:
			return 0.02*year;
		case WORST:
			return a*pow(alpha*second, n);
		case NORMAL:		//暫時把NORMAL和WORST改成相同
			return a*pow(alpha*second, n);
		case BEST:
			return a*pow(0.25*second, n);
		default:
			return a*pow(alpha*second, n);
		}
	}
	switch (status){
	case DCC_M:
	case DCC_NONE:
		if (year - (double)y > 0.00001)
			return (Rate[y + 1][1] - Rate[y][1]) *(year - (double)y) + Rate[y][1];
		return Rate[y][1];
		break;
	case DCC_F:
		if (year - (double)y > 0.00001)
			return (Rate[y + 1][2] - Rate[y][2]) *(year - (double)y) + Rate[y][2];
		return Rate[y][2];
	case DCC_S:
		return Rate[y][0];
		if (year - (double)y > 0.00001)
			return (Rate[y + 1][0] - Rate[y][0]) *(year - (double)y) + Rate[y][0];
	case FF:
		return 0.02*year;
	case WORST:
		return a*pow(alpha*second, n);
	case NORMAL:		//暫時把NORMAL和WORST改成相同
		return a*pow(alpha*second, n);
	case BEST:
		return a*pow(0.25*second, n);
	default:
		return a*pow(alpha*second, n);
	}
}

