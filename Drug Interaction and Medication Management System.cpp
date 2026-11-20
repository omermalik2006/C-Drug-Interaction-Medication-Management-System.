#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <limits>
#include <algorithm>
#include <cctype>
#include <cmath>
using namespace std;

struct QueueNode {
    int data; QueueNode* next;
    QueueNode(int value) : data(value), next(NULL) {}
};
class IntQueue {
    QueueNode* first; QueueNode* last;
public:
    IntQueue() : first(NULL), last(NULL) {}
    ~IntQueue() { while (!empty()) pop(); }
    bool empty() const { return first == NULL; }
    void push(int value) {
        QueueNode* n = new QueueNode(value);
        if (!last) first = last = n;
        else { last->next = n; last = n; }
    }
    int front() const { return empty() ? -1 : first->data; }
    void pushFront(int value) {
        QueueNode* n = new QueueNode(value);
        if (!first) first = last = n;
        else { n->next = first; first = n; }
    }
    void pop() {
        if (empty()) return;
        QueueNode* t = first; first = first->next;
        if (!first) last = NULL;
        delete t;
    }
};

struct StackNode {
    int data; StackNode* next;
    StackNode(int value, StackNode* link) : data(value), next(link) {}
};
class IntStack {
    StackNode* topNode;
public:
    IntStack() : topNode(NULL) {}
    ~IntStack() { while (!empty()) pop(); }
    bool empty() const { return topNode == NULL; }
    void push(int value) { topNode = new StackNode(value, topNode); }
    int top() const { return empty() ? -1 : topNode->data; }
    void pop() {
        if (empty()) return;
        StackNode* t = topNode; topNode = topNode->next; delete t;
    }
};

struct Patient {
    int id, age; string name; Patient* next;
    Patient(int i, const string& n, int a) : id(i), age(a), name(n), next(NULL) {}
};
struct Medicine {
    int id; string name, generic, category; double brandPrice, genericPrice;
};
struct Prescription {
    int id, patientId, medicineId; string instructions; bool processed;
};
struct Interaction {
    int medicineA, medicineB; string severity, note;
};

string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}
bool safeField(const string& s) {
    return s.find('|') == string::npos && s.find('\n') == string::npos && s.find('\r') == string::npos;
}
vector<string> split(const string& line, char delimiter) {
    vector<string> parts; string part; stringstream ss(line);
    while (getline(ss, part, delimiter)) parts.push_back(part);
    if (!line.empty() && line[line.size()-1] == delimiter) parts.push_back("");
    return parts;
}
bool parseInt(const string& s, int& value) {
    stringstream ss(s); ss >> value;
    return !ss.fail() && ss.peek() == EOF;
}
bool parseDouble(const string& s, double& value) {
    stringstream ss(s); ss >> value;
    return !ss.fail() && ss.peek() == EOF && value >= 0 && isfinite(value);
}
string lowerText(string s) {
    for (size_t i=0; i<s.size(); ++i) s[i] = (char)tolower((unsigned char)s[i]);
    return s;
}

class InteractionGraph {
    vector< vector<int> > adjacency;
public:
    void build(int count, const vector<Interaction>& records) {
        adjacency.assign(count, vector<int>());
        for (size_t i=0; i<records.size(); ++i) {
            int a = records[i].medicineA, b = records[i].medicineB;
            if (a >= 0 && b >= 0 && a < count && b < count && a != b) {
                adjacency[a].push_back(b); adjacency[b].push_back(a);
            }
        }
    }
    const vector<int>& neighbors(int id) const {
        static const vector<int> emptyList;
        if (id < 0 || id >= (int)adjacency.size()) return emptyList;
        return adjacency[id];
    }
    vector<int> bfs(int start) const {
        vector<int> order;
        if (start < 0 || start >= (int)adjacency.size()) return order;
        vector<bool> visited(adjacency.size(), false);
        queue<int> q; q.push(start); visited[start] = true;
        while (!q.empty()) {
            int u=q.front(); q.pop(); order.push_back(u);
            for (size_t i=0; i<adjacency[u].size(); ++i) {
                int v=adjacency[u][i];
                if (!visited[v]) { visited[v]=true; q.push(v); }
            }
        }
        return order;
    }
    vector<int> dfs(int start) const {
        vector<int> order;
        if (start < 0 || start >= (int)adjacency.size()) return order;
        vector<bool> visited(adjacency.size(), false);
        vector<int> st; st.push_back(start);
        while (!st.empty()) {
            int u=st.back(); st.pop_back();
            if (visited[u]) continue;
            visited[u]=true; order.push_back(u);
            for (int i=(int)adjacency[u].size()-1; i>=0; --i)
                if (!visited[adjacency[u][i]]) st.push_back(adjacency[u][i]);
        }
        return order;
    }
};

class System {
    Patient* head;
    vector<Medicine> medicines;
    vector<Prescription> prescriptions;
    vector<Interaction> interactions;
    IntQueue pending;
    IntStack undo;
    int nextPatient, nextMedicine, nextPrescription;
    InteractionGraph graph;

    void pause() {
        cout << "\nPress ENTER to continue...";
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    string readLine(const string& prompt) {
        string value; cout << prompt; getline(cin, value); return trim(value);
    }
    bool readInt(const string& prompt, int& value) {
        cout << prompt;
        if (!(cin >> value)) {
            cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a whole number.\n"; return false;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); return true;
    }
    bool readPrice(const string& prompt, double& value) {
        cout << prompt;
        if (!(cin >> value) || value < 0 || !isfinite(value)) {
            cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid price. Enter a non-negative number.\n"; return false;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); return true;
    }
    int patientIndex(int id) const {
        Patient* p=head; while (p) { if (p->id==id) return id; p=p->next; }
        return -1;
    }
    int medicineIndexById(int id) const {
        for (size_t i=0;i<medicines.size();++i) if (medicines[i].id==id) return (int)i;
        return -1;
    }
    int medicineIndexByName(const string& name) const {
        string target=lowerText(trim(name));
        for (size_t i=0;i<medicines.size();++i)
            if (lowerText(medicines[i].name)==target) return (int)i;
        return -1;
    }
    int prescriptionIndex(int id) const {
        for (size_t i=0;i<prescriptions.size();++i) if (prescriptions[i].id==id) return (int)i;
        return -1;
    }
    void rebuildGraph() { graph.build(nextMedicine, interactions); }
    void printHeader(const string& title) const {
        cout << "\n+----------------------------------------------------------+\n";
        cout << "| " << left << setw(56) << title << "|\n";
        cout << "+----------------------------------------------------------+\n";
    }
    void showCategories() const {
        cout << " 1. Analgesic / Antipyretic\n 2. NSAID\n 3. Antibiotic\n"
             << " 4. Antihistamine\n 5. Proton Pump Inhibitor\n 6. Antacid\n"
             << " 7. Antidiabetic\n 8. Antihypertensive\n 9. Other\n";
    }
    string categoryName(int choice) const {
        switch(choice) {
            case 1: return "Analgesic / Antipyretic"; case 2: return "NSAID";
            case 3: return "Antibiotic"; case 4: return "Antihistamine";
            case 5: return "Proton Pump Inhibitor"; case 6: return "Antacid";
            case 7: return "Antidiabetic"; case 8: return "Antihypertensive";
            default: return "Other";
        }
    }
    void addPatient() {
        printHeader("ADD PATIENT");
        string name=readLine("Patient name: ");
        if (name.empty() || !safeField(name)) { cout << "Name is required; | is not allowed.\n"; return; }
        int age; if (!readInt("Age: ",age)) return;
        if (age<0 || age>120) { cout << "Age must be between 0 and 120.\n"; return; }
        Patient* p=new Patient(nextPatient++,name,age);
        if (!head) head=p;
        else { Patient* cur=head; while(cur->next) cur=cur->next; cur->next=p; }
        cout << "Patient added successfully. Assigned ID: " << p->id << "\n";
    }
    void listPatients() const {
        printHeader("PATIENT DIRECTORY");
        if (!head) { cout << "No patient records found.\n"; return; }
        cout << left << setw(8) << "ID" << setw(32) << "NAME" << "AGE\n";
        cout << "----------------------------------------------------------\n";
        for(Patient* p=head;p;p=p->next) cout << left << setw(8) << p->id << setw(32) << p->name << p->age << "\n";
    }
    void addMedicine() {
        printHeader("ADD MEDICINE");
        Medicine m; m.name=readLine("Medicine name (e.g. Paracetamol): ");
        m.generic=readLine("Generic / alternative name: ");
        if(m.name.empty() || m.generic.empty() || !safeField(m.name) || !safeField(m.generic)) {
            cout << "Both names are required; | is not allowed.\n"; return;
        }
        if(medicineIndexByName(m.name)!=-1) { cout << "A medicine with this name already exists.\n"; return; }
        cout << "Select category:\n"; showCategories();
        int c; if(!readInt("Category number: ",c)) return;
        if(c<1 || c>9) { cout << "Choose a category from 1 to 9.\n"; return; }
        m.category=categoryName(c);
        if(!readPrice("Brand price: ",m.brandPrice)) return;
        if(!readPrice("Generic / alternative price: ",m.genericPrice)) return;
        m.id=nextMedicine++; medicines.push_back(m); rebuildGraph();
        cout << "Medicine added. Medicine ID: " << m.id << "\n";
    }
    void listMedicines() const {
        printHeader("MEDICINE CATALOGUE");
        if(medicines.empty()) { cout << "No medicines registered.\n"; return; }
        cout << left << setw(6) << "ID" << setw(23) << "MEDICINE" << setw(25) << "CATEGORY" << "BRAND / ALT.\n";
        cout << "--------------------------------------------------------------------------\n";
        for(size_t i=0;i<medicines.size();++i) {
            const Medicine& m=medicines[i];
            cout << left << setw(6) << m.id << setw(23) << m.name.substr(0,21)
                 << setw(25) << m.category.substr(0,23) << fixed << setprecision(2)
                 << m.brandPrice << " / " << m.genericPrice << "\n";
        }
    }
    void browseCategories() const {
        printHeader("BROWSE MEDICINES BY CATEGORY");
        if(medicines.empty()) { cout << "No medicines registered.\n"; return; }
        for(int c=1;c<=9;++c) {
            string category=categoryName(c); bool found=false;
            for(size_t i=0;i<medicines.size();++i) if(medicines[i].category==category) {
                if(!found) cout << "\n[ " << category << " ]\n";
                found=true; cout << "  - " << medicines[i].name << " (" << medicines[i].generic << ")\n";
            }
        }
    }
    void comparePrices() {
        printHeader("LOWEST ALTERNATIVE PRICE FIRST");
        if(medicines.empty()) { cout << "Add medicines first.\n"; return; }
        priority_queue< pair<double,int>, vector< pair<double,int> >, greater< pair<double,int> > > prices;
        for(size_t i=0;i<medicines.size();++i) prices.push(make_pair(medicines[i].genericPrice,(int)i));
        while(!prices.empty()) {
            int i=prices.top().second; cout << left << setw(24) << medicines[i].name
                << "Alternative price: " << fixed << setprecision(2) << prices.top().first << "\n";
            prices.pop();
        }
        cout << "Price sorting is informational only; suitability must be confirmed by a qualified professional.\n";
    }
    void addPrescription() {
        printHeader("CREATE PRESCRIPTION RECORD");
        int pid; if(!readInt("Patient ID: ",pid)) return;
        if(patientIndex(pid)==-1) { cout << "Patient ID not found. Add the patient first.\n"; return; }
        int mid; if(!readInt("Medicine ID: ",mid)) return;
        int mi=medicineIndexById(mid); if(mi==-1) { cout << "Medicine ID not found.\n"; return; }
        string instructions=readLine("Prescription instructions (entered by authorized user): ");
        if(instructions.empty() || !safeField(instructions)) { cout << "Instructions are required; | is not allowed.\n"; return; }
        Prescription p; p.id=nextPrescription++; p.patientId=pid; p.medicineId=mid;
        p.instructions=instructions; p.processed=false; prescriptions.push_back(p); pending.push(p.id);
        cout << "Prescription record created. ID: " << p.id << "\n";
    }
    void listPrescriptions() const {
        printHeader("PRESCRIPTION QUEUE / RECORDS");
        if(prescriptions.empty()) { cout << "No prescription records.\n"; return; }
        for(size_t i=0;i<prescriptions.size();++i) {
            const Prescription& p=prescriptions[i]; int mi=medicineIndexById(p.medicineId);
            cout << "Record #" << p.id << " | Patient #" << p.patientId << " | Medicine: "
                 << (mi==-1 ? "[missing]" : medicines[mi].name) << "\n"
                 << "Instructions: " << p.instructions << "\nStatus: "
                 << (p.processed?"Processed":"Pending") << "\n------------------------------\n";
        }
    }
    void processPrescription() {
        printHeader("PROCESS NEXT PRESCRIPTION");
        while(!pending.empty()) {
            int idx=prescriptionIndex(pending.front());
            if(idx>=0 && !prescriptions[idx].processed) break;
            pending.pop();
        }
        if(pending.empty()) { cout << "There are no pending prescriptions.\n"; return; }
        int id=pending.front(); pending.pop(); int idx=prescriptionIndex(id);
        if(idx<0) { cout << "Queue record is invalid.\n"; return; }
        prescriptions[idx].processed=true; undo.push(id);
        cout << "Prescription #" << id << " marked as processed.\n";
    }
    void undoProcess() {
        printHeader("UNDO LAST PROCESSING ACTION");
        if(undo.empty()) { cout << "There is no processing action to undo.\n"; return; }
        int id=undo.top(); undo.pop(); int idx=prescriptionIndex(id);
        if(idx<0) { cout << "The referenced record no longer exists.\n"; return; }
        prescriptions[idx].processed=false; pending.pushFront(id);
        cout << "Processing status for prescription #" << id << " has been reverted.\n";
    }
    int findInteraction(int a,int b) const {
        for(size_t i=0;i<interactions.size();++i)
            if((interactions[i].medicineA==a && interactions[i].medicineB==b) ||
               (interactions[i].medicineA==b && interactions[i].medicineB==a)) return (int)i;
        return -1;
    }
    void addInteraction() {
        printHeader("ADD VERIFIED INTERACTION RECORD");
        cout << "Only enter records from a verified, instructor-approved educational source.\n";
        int a,b; if(!readInt("First medicine ID: ",a)) return;
        if(!readInt("Second medicine ID: ",b)) return;
        int ia=medicineIndexById(a), ib=medicineIndexById(b);
        if(ia==-1 || ib==-1) { cout << "Both medicine IDs must exist in the catalogue.\n"; return; }
        if(a==b) { cout << "Select two different medicines.\n"; return; }
        if(findInteraction(a,b)!=-1) { cout << "An interaction record already exists for this pair.\n"; return; }
        cout << "Severity label in your source (e.g. Low / Moderate / High): ";
        string severity=readLine("");
        string note=readLine("Source note / reference (brief): ");
        if(severity.empty() || note.empty() || !safeField(severity) || !safeField(note)) {
            cout << "Severity and source note are required; | is not allowed.\n"; return;
        }
        Interaction x; x.medicineA=a; x.medicineB=b; x.severity=severity; x.note=note;
        interactions.push_back(x); rebuildGraph();
        cout << "Interaction record added to the graph.\n";
    }
    void showInteractionRecords() const {
        printHeader("RECORDED MEDICINE INTERACTIONS");
        if(interactions.empty()) { cout << "No interaction records are currently stored.\n"; return; }
        for(size_t i=0;i<interactions.size();++i) {
            int a=medicineIndexById(interactions[i].medicineA), b=medicineIndexById(interactions[i].medicineB);
            cout << (a==-1?"[missing]":medicines[a].name) << " <--> "
                 << (b==-1?"[missing]":medicines[b].name) << " | " << interactions[i].severity
                 << "\nSource note: " << interactions[i].note << "\n------------------------------\n";
        }
        cout << "These are stored dataset entries, not an independently generated medical assessment.\n";
    }
    void checkMedicinePair() {
        printHeader("CHECK TWO MEDICINES IN STORED DATASET");
        int a,b;
        if(!readInt("First medicine ID: ",a)) return;
        if(!readInt("Second medicine ID: ",b)) return;
        int ia=medicineIndexById(a), ib=medicineIndexById(b);
        if(ia==-1 || ib==-1) { cout << "Both medicine IDs must exist in the catalogue.\n"; return; }
        if(a==b) { cout << "Choose two different medicines.\n"; return; }
        int record=findInteraction(a,b);
        cout << "Pair: " << medicines[ia].name << " + " << medicines[ib].name << "\n";
        if(record==-1) {
            cout << "No interaction record for this pair exists in the currently loaded dataset.\n";
            cout << "This does NOT prove that the combination is safe or has no interaction.\n";
        } else {
            const Interaction& x=interactions[record];
            cout << "Matching dataset record found. Source severity label: " << x.severity << "\n";
            cout << "Source note: " << x.note << "\n";
            cout << "Verify the information with a qualified healthcare professional.\n";
        }
    }
    void traverseInteractions(bool useBfs) {
        printHeader(useBfs?"BFS INTERACTION NETWORK TRAVERSAL":"DFS INTERACTION NETWORK TRAVERSAL");
        int id; if(!readInt("Starting medicine ID: ",id)) return;
        int idx=medicineIndexById(id); if(idx==-1) { cout << "Medicine ID not found.\n"; return; }
        vector<int> order=useBfs?graph.bfs(id):graph.dfs(id);
        if(order.size()==1 && graph.neighbors(id).empty()) {
            cout << "No interaction edges are recorded for " << medicines[idx].name << ".\n"; return;
        }
        cout << "Traversal order: ";
        for(size_t i=0;i<order.size();++i) {
            int mi=medicineIndexById(order[i]);
            if(mi!=-1) cout << medicines[mi].name << (i+1<order.size()?" -> ":"");
        }
        cout << "\nTraversal visits medicines connected in the stored interaction graph.\n";
    }
    void searchMedicine() {
        printHeader("SEARCH MEDICINE");
        string key=lowerText(readLine("Enter medicine name: "));
        if(key.empty()) { cout << "Enter a search term.\n"; return; }
        bool found=false;
        for(size_t i=0;i<medicines.size();++i) if(lowerText(medicines[i].name).find(key)!=string::npos || lowerText(medicines[i].generic).find(key)!=string::npos) {
            found=true; cout << "ID: " << medicines[i].id << " | " << medicines[i].name
                << " | " << medicines[i].category << " | Generic: " << medicines[i].generic << "\n";
        }
        if(!found) cout << "No matching medicine found.\n";
    }
    void save() const {
        ofstream pf("patients.txt"), mf("medicines.txt"), rf("prescriptions.txt"), xf("interactions.txt");
        if(!pf || !mf || !rf || !xf) { cout << "ERROR: Could not open one or more data files for saving.\n"; return; }
        for(Patient* p=head;p;p=p->next) pf << p->id << "|" << p->name << "|" << p->age << "\n";
        pf.close();
        for(size_t i=0;i<medicines.size();++i) {
            const Medicine& m=medicines[i]; mf << m.id << "|" << m.name << "|" << m.generic << "|" << m.category << "|" << setprecision(15) << m.brandPrice << "|" << m.genericPrice << "\n";
        }
        mf.close();
        for(size_t i=0;i<prescriptions.size();++i) {
            const Prescription& p=prescriptions[i]; rf << p.id << "|" << p.patientId << "|" << p.medicineId << "|" << p.instructions << "|" << (p.processed?1:0) << "\n";
        }
        rf.close();
        for(size_t i=0;i<interactions.size();++i) {
            const Interaction& x=interactions[i]; xf << x.medicineA << "|" << x.medicineB << "|" << x.severity << "|" << x.note << "\n";
        }
        xf.close();
        if(!pf.good() || !mf.good() || !rf.good() || !xf.good()) cout << "WARNING: A file write may not have completed.\n";
        else cout << "All records saved successfully.\n";
    }
    void load() {
        string line; int a,b,c; double d,e;
        ifstream pf("patients.txt");
        while(getline(pf,line)) { vector<string> v=split(line,'|');
            if(v.size()!=3 || !parseInt(v[0],a) || !parseInt(v[2],b) || a<=0 || b<0 || b>120 || patientIndex(a)!=-1 || trim(v[1]).empty()) continue;
            Patient* p=new Patient(a,v[1],b); if(!head) head=p; else { Patient* t=head; while(t->next)t=t->next; t->next=p; }
            if(a>=nextPatient) nextPatient=a+1;
        }
        ifstream mf("medicines.txt");
        while(getline(mf,line)) { vector<string> v=split(line,'|');
            if(v.size()!=6 || !parseInt(v[0],a) || !parseDouble(v[4],d) || !parseDouble(v[5],e) || a<=0 || medicineIndexById(a)!=-1 || trim(v[1]).empty()) continue;
            Medicine m; m.id=a; m.name=v[1]; m.generic=v[2]; m.category=v[3]; m.brandPrice=d; m.genericPrice=e; medicines.push_back(m);
            if(a>=nextMedicine) nextMedicine=a+1;
        }
        ifstream rf("prescriptions.txt");
        while(getline(rf,line)) { vector<string> v=split(line,'|');
            if(v.size()!=5 || !parseInt(v[0],a) || !parseInt(v[1],b) || !parseInt(v[2],c) || (v[4]!="0" && v[4]!="1") || a<=0 || patientIndex(b)==-1 || medicineIndexById(c)==-1 || prescriptionIndex(a)!=-1) continue;
            Prescription p; p.id=a; p.patientId=b; p.medicineId=c; p.instructions=v[3]; p.processed=(v[4]=="1"); prescriptions.push_back(p);
            if(a>=nextPrescription) nextPrescription=a+1;
        }
        ifstream xf("interactions.txt");
        while(getline(xf,line)) { vector<string> v=split(line,'|');
            if(v.size()!=4 || !parseInt(v[0],a) || !parseInt(v[1],b) || a==b || medicineIndexById(a)==-1 || medicineIndexById(b)==-1 || findInteraction(a,b)!=-1) continue;
            Interaction x; x.medicineA=a; x.medicineB=b; x.severity=v[2]; x.note=v[3]; interactions.push_back(x);
        }
        for(size_t i=0;i<prescriptions.size();++i) if(!prescriptions[i].processed) pending.push(prescriptions[i].id);
        rebuildGraph();
    }
    void listPatientPrescriptions(int patientId) const {
        printHeader("MY PRESCRIPTION HISTORY");
        bool found=false;
        for(size_t i=0;i<prescriptions.size();++i) {
            const Prescription& p=prescriptions[i];
            if(p.patientId!=patientId) continue;
            found=true;
            int mi=medicineIndexById(p.medicineId);
            cout << "Prescription #" << p.id << " | Medicine: "
                 << (mi==-1?"[missing]":medicines[mi].name)
                 << " | Status: " << (p.processed?"Processed":"Pending")
                 << "\nInstructions: " << p.instructions << "\n------------------------------\n";
        }
        if(!found) cout << "No prescriptions are linked to this patient ID.\n";
    }
    void doctorDashboard() {
        int choice=-1;
        while(choice!=0) {
            printHeader("DOCTOR PORTAL");
            cout << " 1. Register a patient\n 2. View patient directory\n"
                 << " 3. Create a prescription\n 4. View all prescriptions\n"
                 << " 5. Browse medicine categories\n 6. Search medicine\n"
                 << " 7. Check two medicines in stored interaction data\n"
                 << " 8. View interaction records\n 9. BFS interaction traversal\n"
                 << "10. DFS interaction traversal\n 0. Back to role selection\n";
            if(!readInt("Choose an option: ",choice)) continue;
            switch(choice) {
                case 1:addPatient();break; case 2:listPatients();break;
                case 3:addPrescription();break; case 4:listPrescriptions();break;
                case 5:browseCategories();break; case 6:searchMedicine();break;
                case 7:checkMedicinePair();break; case 8:showInteractionRecords();break;
                case 9:traverseInteractions(true);break; case 10:traverseInteractions(false);break;
                case 0:break; default:cout << "Choose an option from 0 to 10.\n";
            }
        }
    }
    void pharmacistDashboard() {
        int choice=-1;
        while(choice!=0) {
            printHeader("PHARMACIST PORTAL");
            cout << " 1. Add medicine to catalogue\n 2. View medicine catalogue\n"
                 << " 3. Browse medicine categories\n 4. Search medicine\n"
                 << " 5. Compare alternative prices\n 6. View prescriptions\n"
                 << " 7. Process next prescription\n 8. Undo last processing\n"
                 << " 9. Add verified interaction record\n10. View interaction records\n"
                 << "11. Check a medicine pair\n12. Save records\n"
                 << " 0. Back to role selection\n";
            if(!readInt("Choose an option: ",choice)) continue;
            switch(choice) {
                case 1:addMedicine();break; case 2:listMedicines();break;
                case 3:browseCategories();break; case 4:searchMedicine();break;
                case 5:comparePrices();break; case 6:listPrescriptions();break;
                case 7:processPrescription();break; case 8:undoProcess();break;
                case 9:addInteraction();break; case 10:showInteractionRecords();break;
                case 11:checkMedicinePair();break; case 12:save();break;
                case 0:break; default:cout << "Choose an option from 0 to 12.\n";
            }
        }
    }
    void patientDashboard() {
        printHeader("PATIENT PORTAL LOGIN");
        int patientId;
        if(!readInt("Enter your patient ID: ",patientId)) return;
        Patient* current=head;
        while(current && current->id!=patientId) current=current->next;
        if(!current) { cout << "Patient ID not found. Please ask the clinic to register you.\n"; return; }
        int choice=-1;
        while(choice!=0) {
            printHeader("PATIENT PORTAL");
            cout << "Welcome, " << current->name << " (Patient #" << current->id << ")\n"
                 << " 1. View my profile\n 2. View medicine catalogue\n"
                 << " 3. Browse medicine categories\n 4. Search medicine\n"
                 << " 5. View my prescription history\n"
                 << " 0. Log out / Back to role selection\n";
            if(!readInt("Choose an option: ",choice)) continue;
            switch(choice) {
                case 1:
                    cout << "Patient ID: " << current->id << "\nName: " << current->name
                         << "\nAge: " << current->age << "\n"; break;
                case 2:listMedicines();break;
                case 3:browseCategories();break;
                case 4:searchMedicine();break;
                case 5:listPatientPrescriptions(patientId);break;
                case 0:break;
                default:cout << "Choose an option from 0 to 5.\n";
            }
        }
    }
public:
    System() : head(NULL), nextPatient(1), nextMedicine(1), nextPrescription(1) { load(); }
    ~System() { while(head) { Patient* t=head; head=head->next; delete t; } }
    void run() {
        int choice=-1;
        while(choice!=0) {
            printHeader("DRUG INTERACTION & MEDICATION MANAGEMENT SYSTEM");
            cout << "Select your portal:\n\n"
                 << " 1. DOCTOR      - Patients, prescriptions and interaction review\n"
                 << " 2. PATIENT     - Profile, medicine catalogue and own prescriptions\n"
                 << " 3. PHARMACIST  - Medicine catalogue, pricing and prescription queue\n"
                 << "\n 0. SAVE & EXIT\n"
                 << "------------------------------------------------------------\n";
            if(!readInt("Enter portal number: ",choice)) continue;
            switch(choice) {
                case 1:doctorDashboard();break;
                case 2:patientDashboard();break;
                case 3:pharmacistDashboard();break;
                case 0:save();cout << "Thank you for using the system. Goodbye!\n";break;
                default:cout << "Please select 0, 1, 2 or 3.\n";
            }
        }
    }
};

int main() {
    cout << "\n\n";
    cout << " \\ \\      / /__| | ___ ___  _ __ ___   ___  | |\n";
    cout << "  \\ \\ /\\ / / _ \\ |/ __/ _ \\| '_ ` _ \\ / _ \\ | |\n";
    cout << "   \\ V  V /  __/ | (_| (_) | | | | | |  __/ |_|\n";
    cout << "    \\_/\\_/ \\___|_|\\___\\___/|_| |_| |_|\\___| (_)\n";
    System app;
    app.run();
    return 0;
}
