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

double AgingRate(AGINGTYPE status, int year){	
	
	double second = year * 365 * 86400;	//0.0039*(0.5*t)^0.2 + 1
	
	switch (status){
	case DCC_M:
	case DCC_NONE:
		return Rate[year][1];
		break;
	case DCC_F:
		return Rate[year][2];
	case DCC_S:
		return Rate[year][0];
	case FF:
		return 0.02*year;
	default:
		return a*pow(alpha*second, n);
	}
}