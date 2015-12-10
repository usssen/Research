
/*路徑讀取測試
string st, ed;
cout << "Press Startpoint & Endpoint of a path" << endl;
while (cin >> st >> ed){
	for (int i = 0; i < PathR.size(); i++){
		if (PathR[i].Gate(0)->GetName() == st && PathR[i].Gate(PathR[i].length() - 1)->GetName() == ed){
			cout << i << endl;
			cout << "CTH,CTE,AT,HT,ST" << endl;
			cout << PathR[i].GetCTH() << ',' << PathR[i].GetCTE() << ',' << PathR[i].GetAT() << ',' << PathR[i].GetHT() << ',' << PathR[i].GetST() << endl;
			GATE* gptr = PathR[i].Gate(0);
			cout << "Clock Path to Startpoint(" << st << ')' << endl;
			for (int j = 0; j < gptr->Clock_Length(); j++)
				cout << gptr->GetClockPath(j)->GetName() << ',' << gptr->GetClockPath(j)->GetInTime() << ',' << gptr->GetClockPath(j)->GetOutTime() << endl;
			cout << "Clock Path to EndPoint(" << ed << ')' << endl;
			gptr = PathR[i].Gate(PathR[i].length() - 1);
			for (int j = 0; j < gptr->Clock_Length(); j++)
				cout << gptr->GetClockPath(j)->GetName() << ',' << gptr->GetClockPath(j)->GetInTime() << ',' << gptr->GetClockPath(j)->GetOutTime() << endl;
			cout << "Path:" << endl;
			for (int j = 0; j < PathR[i].length(); j++)
				cout << PathR[i].Gate(j)->GetName() << ',' << PathR[i].In_time(j) << ',' << PathR[i].Out_time(j) << endl;
			cout << endl;
			//break;
		}
	}
}
*/

/*測試電路讀取用
string WireName;
while (cin >> WireName){
WIRE* w = Circuit[0].GetWire(WireName);
cout << w->GetInput()->GetName() << endl;
int n = w->No_Output();
for (int i = 0; i < n; i++)
cout << w->GetOutput(i)->GetName() << endl;
}
*/
/*
int k = 0;
for (unsigned i = 0; i < PathR.size(); i++){
	if (Choice[i]){
		cout << i << ' ';
		k++;
	}
}
cout << endl << k << endl;
*/

//if (stptr->GetName() == "u2_uk_K_r14_reg_19__u0" && edptr->GetName() == "g190305_u0_o")
//

//cout << "Start : " << pptr->Gate(0)->GetName() << " End : " << pptr->Gate(pptr->length() - 1)->GetName() << endl;
//cout << "Put DCC On : " << "NONE" << ' ' << edptr->GetClockPath(j)->GetName() << endl << "DCC TYPE" << x << endl;

/*
if (Vio_Check(pptr, stn, edn, ast, aed, year - 1)){
cout<<stptr->GetName() << ' ' << edptr->GetName() << endl;
cout << clks << ' ' << Tcq << ' ' << DelayP << ' ' << clkt << ' ' << pptr->GetST() << ' ' << period << endl << pptr->GetCTH() << endl;
}
*/


/*
int m, n, i;
double* pData;
m = n = PathC.size();
EdgeA = (double **)malloc(m*sizeof(double *)+m*n*sizeof(double));	//m個陣列開頭+m*n個空間
for (i = 0, pData = (double *)(EdgeA + m); i < m; i++, pData += n)	//前m(double*)個為陣列開頭,之後每個開頭(edgea[i])的位置相隔n(double)
EdgeA[i] = pData;
EdgeB = (double **)malloc(m*sizeof(double *)+m*n*sizeof(double));
for (i = 0, pData = (double *)(EdgeB + m); i < m; i++, pData += n)
EdgeB[i] = pData;
cor = (double **)malloc(m*sizeof(double *)+m*n*sizeof(double));
for (i = 0, pData = (double *)(cor + m); i < m; i++, pData += n)
cor[i] = pData;
*/


/*
for (int i = 0; i < ss; i++){
for (int j = i; j < ss; j++){
if (j == i){	//y = x
EdgeA[i][j] = 1;
EdgeB[i][j] = 0;
cor[i][j] = 0;
continue;
}
double a = (double)rand() / RAND_MAX*0.3 + 0.85;	//差距大概在15%
double b = 0.0;// (double)rand() / RAND_MAX - 0.5;	//-0.5~0.5
double c = (double)rand() * 2 / RAND_MAX - 1;
EdgeA[i][j] = a;		// y = ax+b
EdgeB[i][j] = b;
EdgeA[j][i] = 1 / a;	//	x = y/a-b/a
EdgeB[j][i] = -1*(b / a);
cor[i][j] = cor[j][i] = c;
}
}
*/