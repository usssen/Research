
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