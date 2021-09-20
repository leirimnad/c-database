#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Group{
    unsigned int code;
    char name[10];
    int startingYear;
    int finishYear;
    int phys;
    int firstSlavePos;
};

struct Student{
    unsigned int code;
    unsigned int groupCode;
    char name[128];
    char educationFunding[64];
    int birthYear;
    int phys;
    int nextSlavePos;
};

struct IndexRecord{
    int masterCode;
    int posInFile;
};

struct IndexRecordNode{
    int masterCode;
    int posInFile;
    struct IndexRecordNode *next;
};

struct StackNode{
    int val;
    struct StackNode *next;
};

struct Stack{
    struct StackNode *top;
    int len;
};

struct getMReturn{
    short found;
    struct Group group;
    long pos;
};

struct getSReturn{
    short found;
    struct Student student;
    long pos;
    long masterPos;
};

void utilM();
struct getMReturn getM(int code);
void getM_user();
void insertM();
void updateM();
void deleteM();
void utilS();
struct getSReturn getS();
void updateS();
void deleteS();
void insertS();
void utilMS();
void downloadIndexTable();
void uploadIndexTable();
void indexTable();
void addToStack();
int popStack();
void readTrashToStack(char filename[], struct Stack *stack);
void writeTrashToFile(char filename[], struct Stack *stack);


const char masterTableFile[] = "groups.fl";
const char indexTableFile[] = "groups.ind";
const char slaveTableFile[] = "students.fl";
const char slaveTrashFile[] = "students.thrash";
const char masterTrashFile[] = "groups.thrash";
FILE* masterTable;
FILE* slaveTable;

struct IndexRecordNode *firstIndexRecord = NULL;
struct Stack slaveTrashIndexes = {NULL, 0};
struct Stack masterTrashIndexes = {NULL, 0};

long fileLen(FILE* f){
    long l = ftell(f);
    fseek(f, 0, SEEK_END);
    long r = ftell(f);
    fseek(f, l, SEEK_SET);
    return r;
}

void printGroup(struct Group gr){
    char sp = ' '; if(!gr.phys) sp = '#';
    printf("%c#%d%c|%c%s%c(%d-%d)%c(slave%c%i)\n", sp, gr.code, sp, sp, gr.name, sp, gr.startingYear, gr.finishYear, sp, sp, gr.firstSlavePos);
}

void printStudent(struct Student st){
    char sp = ' '; if(!st.phys) sp = '#';
    printf("%c#%d%c|%cgr.%c%u%c|%c%s%c(%d,%c%s)%c(n.sl.%c%i)\n", sp, st.code, sp, sp, sp, st.groupCode, sp, sp, st.name, sp, st.birthYear, sp, st.educationFunding, sp, sp, st.nextSlavePos);
}

int main() {
    downloadIndexTable();
    masterTable = fopen(masterTableFile, "r+b");
    slaveTable = fopen(slaveTableFile, "r+b");
    if(!masterTable) {
        fopen(masterTableFile, "w");
        fclose(masterTable);
        masterTable = fopen(masterTableFile, "r+b");
    }
    if(!slaveTable) {
        fopen(slaveTableFile, "w");
        fclose(slaveTable);
        slaveTable = fopen(slaveTableFile, "r+b");
    }

    readTrashToStack(slaveTrashFile, &slaveTrashIndexes);
    readTrashToStack(masterTrashFile, &masterTrashIndexes);


    char command[50] = "";
    while (1){
        printf("\nEnter the command or 'stop':");
        scanf("%s", command);

        if(strcmp(command, "stop") == 0) break;
        else if(strcmp(command, "util-m") == 0) utilM();
        else if(strcmp(command, "get-m") == 0) getM_user();
        else if(strcmp(command, "insert-m") == 0) insertM();
        else if(strcmp(command, "update-m") == 0) updateM();
        else if(strcmp(command, "delete-m") == 0) deleteM();
        else if(strcmp(command, "util-s") == 0) utilS();
        else if(strcmp(command, "util-ms") == 0) utilMS();
        else if(strcmp(command, "get-s") == 0) getS();
        else if(strcmp(command, "insert-s") == 0) insertS();
        else if(strcmp(command, "update-s") == 0) updateS();
        else if(strcmp(command, "delete-s") == 0) deleteS();
        else if(strcmp(command, "index-table") == 0) indexTable();
        else printf("! Unknown command !\n");
    }

    writeTrashToFile(slaveTrashFile, &slaveTrashIndexes);
    writeTrashToFile(masterTrashFile, &masterTrashIndexes);

    fclose(slaveTable);
    fclose(masterTable);
    uploadIndexTable();
    return 0;
}

void downloadIndexTable(){
    FILE* indexTable = fopen(indexTableFile, "rb");
    long len = fileLen(indexTable);
    struct IndexRecordNode *prev = NULL;
    while (ftell(indexTable) < len) {
        struct IndexRecord rec;
        fread(&rec, sizeof(struct IndexRecord), 1, indexTable);
        struct IndexRecordNode newNode = {rec.masterCode, rec.posInFile, NULL};
        struct IndexRecordNode *newNodePtr = malloc(sizeof(struct IndexRecordNode));
        *newNodePtr = newNode;
        if(prev != NULL){
            prev->next = newNodePtr;
        } else {
            firstIndexRecord = newNodePtr;
        }
        prev = newNodePtr;
    }
    fclose(indexTable);
    printf("Index table downloaded.\n");
}

void uploadIndexTable(){
    FILE* indexTable = fopen(indexTableFile, "wb");
    struct IndexRecordNode *cur = firstIndexRecord;
    while (cur != NULL){
        struct IndexRecord n = {cur->masterCode, cur->posInFile};
        fwrite ( &n, sizeof(struct IndexRecord), 1, indexTable );
        cur = cur->next;
    }
    fclose(indexTable);
    printf("Index table uploaded.\n");
}

void getM_user(){
    printf("> Enter the code of the group:");
    unsigned int code;
    scanf("%u", &code);
    struct getMReturn ret = getM(code);
    if (ret.found == 0){
        printf("! Record not found !\n");
        return;
    }

    printGroup(ret.group);
}

struct getMReturn getM(int code){
    struct IndexRecordNode *cur = firstIndexRecord;
    while (cur != NULL && cur->masterCode != code) cur = cur->next;

    if(cur == NULL){
        struct Group gr = {0, "NOT FOUND", 0, 0, 0, -1};
        struct getMReturn ret = {0, gr, -1};
        return ret;
    }

    fseek(masterTable, cur->posInFile * sizeof(struct Group), SEEK_SET);
    struct Group group;
    fread(&group, sizeof(struct Group), 1, masterTable);
    struct getMReturn ret = {1, group, cur->posInFile};
    return ret;
}

void insertM(){
    // get code
    int code = 1;
    struct IndexRecordNode *cur = firstIndexRecord;
    while(cur != NULL){
        if(cur->masterCode >= code) code = cur->masterCode + 1;
        cur = cur->next;
    }

    // get data
    char name[10];
    int sty, fy;
    printf("> Enter the name of the group:");
    scanf("%s", name);
    printf("> Enter the first year of the education:");
    scanf("%i", &sty);
    printf("> Enter the last year of the education:");
    scanf("%i", &fy);
    struct Group new = {code, "", sty, fy, 1, -1};
    strcpy(new.name, name);

    // writing

    fseek(masterTable, 0, SEEK_SET);
    long pos = 0;
    struct Group read;
    long len = fileLen(masterTable);

    if(masterTrashIndexes.len > 0){
        // записать на место удаленного
        pos = popStack(&masterTrashIndexes);
        printf("Deleted record was replaced with new one.\n");
    } else {
        pos = len/sizeof(struct Group);
    }

    fseek(masterTable, (signed long) sizeof(struct Group) * pos, SEEK_SET);
    fwrite(&new, sizeof(struct Group), 1, masterTable);


    // add to index table
    cur = firstIndexRecord; struct IndexRecordNode *prev = NULL;
    while (cur != NULL && cur->masterCode < code) {
        prev = cur;
        cur = cur->next;
    }

    struct IndexRecordNode newNode = {code, pos, cur};
    struct IndexRecordNode *newNodePtr = malloc(sizeof(struct IndexRecordNode));
    *newNodePtr =  newNode;
    if(firstIndexRecord == NULL) firstIndexRecord = newNodePtr;
    if(prev != NULL) prev->next = newNodePtr;

    printf("Successfully created with code #%d!\n", code);
}

void updateM(){
    printf("> Enter the code of the group:");
    unsigned int code;
    scanf("%u", &code);

    struct IndexRecordNode *cur = firstIndexRecord;
    while (cur != NULL && cur->masterCode != code) cur = cur->next;
    if(cur == NULL){
        printf("! Record not found !\n");
        return;
    }
    fseek(masterTable, cur->posInFile * sizeof(struct Group), SEEK_SET);
    struct Group group;
    fread(&group, sizeof(struct Group), 1, masterTable);

    printf("UPDATING: ");
    printGroup(group);

    // get data
    printf("> Enter the new name of the group (previously - %s):", group.name);
    scanf("%s", group.name);
    printf("> Enter the new first year of the education (previously - %i):", group.startingYear);
    scanf("%i", &group.startingYear);
    printf("> Enter the last year of the education (previously - %i):", group.finishYear);
    scanf("%i", &group.finishYear);

    fseek(masterTable, cur->posInFile * sizeof(struct Group), SEEK_SET);
    fwrite(&group, sizeof(struct Group), 1, masterTable);

    printf("Successfully updated!\n");
}

void deleteM(){
    printf("> Enter the code of the group:");
    unsigned int code;
    scanf("%u", &code);

    struct IndexRecordNode *cur = firstIndexRecord;
    while (cur != NULL && cur->masterCode != code) cur = cur->next;
    if(cur == NULL){
        printf("! Record not found !\n");
        return;
    }
    fseek(masterTable, cur->posInFile * sizeof(struct Group), SEEK_SET);
    struct Group group;
    fread(&group, sizeof(struct Group), 1, masterTable);

    printf("DELETING: ");
    printGroup(group);
    printf("Are you sure? (Y/N) ");
    char ans[50];
    scanf("%s", ans);
    if(strcmp(ans, "Y") != 0) return;

    addToStack(&masterTrashIndexes, cur->posInFile);
    group.phys = 0;
    fseek(masterTable, cur->posInFile * sizeof(struct Group), SEEK_SET);
    fwrite(&group, sizeof(struct Group), 1, masterTable);

    // deleting slaves when deleting master
    int nextSlave = group.firstSlavePos;
    int c = 0;
    while (nextSlave != -1){
        fseek(slaveTable, nextSlave * sizeof(struct Student), SEEK_SET);
        struct Student st;
        fread(&st, sizeof(struct Student), 1, slaveTable);

        addToStack(&slaveTrashIndexes, nextSlave);

        st.phys = 0; nextSlave = st.nextSlavePos; st.nextSlavePos = -1;
        fseek(slaveTable, -1 * sizeof(struct Student), SEEK_CUR);
        fwrite(&st, sizeof(struct Student), 1, slaveTable);
        c++;
    }

    cur = firstIndexRecord; struct IndexRecordNode *prev = NULL;
    while (cur->masterCode != code) { prev = cur; cur = cur->next; }
    if(prev) prev->next = cur->next;
    if(cur == firstIndexRecord) firstIndexRecord = cur->next;

    printf("Successfully deleted.\n");
    if(c) printf("! %d students were deleted with the group !\n", c);

}

void utilM(){
    int c = 0;
    fseek(masterTable, 0, SEEK_SET);
    long len = fileLen(masterTable);
    while (ftell(masterTable) < len) {
        struct Group gr;
        fread(&gr, sizeof(struct Group), 1, masterTable);
        printGroup(gr);
        if(gr.phys) c++;
    }
    if(c==0) printf("No records found!\n");
    else printf("Total: %d groups\n", c);
}

void indexTable(){
    int code = 1;
    struct IndexRecordNode *cur = firstIndexRecord;
    printf("Index table:\n");
    while(cur != NULL){
        printf("#%d - pos.%d\n", cur->masterCode, cur->posInFile);
        cur = cur->next;
    }
}


void utilS(){
    int c = 0;
    fseek(slaveTable, 0, SEEK_SET);
    long len = fileLen(slaveTable);
    while (ftell(slaveTable) < len) {
        struct Student st;
        fread(&st, sizeof(struct Student), 1, slaveTable);
        printStudent(st);
        if(st.phys) c++;
    }
    if(c==0) printf("No records found!\n");
    else printf("Total: %d students\n", c);
}

void insertS(){

    // get code
    unsigned int code = 1;

    fseek(slaveTable, 0, SEEK_SET);
    long len = fileLen(slaveTable);
    while (ftell(slaveTable) < len){
        struct Student read;
        fread(&read, sizeof(struct Student), 1, slaveTable);
        if(read.code >= code && read.phys) code = read.code + 1;
    }

    // get data

    char name[100], funding[50];
    int birthYear, grcode;
    printf("> Enter the student's name:");
    scanf("%s", name);

    short found = 0;
    struct Group masterGroup;
    long masterpos = 0;
    while (!found){
        printf("> Enter the group code or -1 to exit:");
        scanf("%i", &grcode);
        if(grcode == -1) return;
        masterpos = 0;
        long masterLen = fileLen(masterTable);
        fseek(masterTable, 0, SEEK_SET);
        while (ftell(masterTable) < masterLen){
            fread(&masterGroup, sizeof(struct Group), 1, masterTable);
            if(masterGroup.code == grcode){
                found = 1;
                break;
            }
            masterpos++;
        }
        if (!found) printf("! Group not found !\n");
    }

    printf("> Enter the way of education funding:");
    scanf("%s", funding);
    printf("> Enter the birth year:");
    scanf("%i", &birthYear);

    struct Student new = {code, grcode, "", "", birthYear, 1, masterGroup.firstSlavePos};
    strcpy(new.name, name);
    strcpy(new.educationFunding, funding);


    // writing slave
    long pos = 0;
    if(slaveTrashIndexes.len > 0){
        // записать на место удаленного
        pos = popStack(&slaveTrashIndexes);
        printf("Deleted record was replaced with new one.\n");
    } else {
        pos = len/sizeof(struct Student);
    }

    fseek(slaveTable, pos * sizeof(struct Student), SEEK_SET);
    fwrite(&new, sizeof(struct Student), 1, slaveTable);


    // writing master
    fseek(masterTable, masterpos * sizeof(struct Group), SEEK_SET);
    masterGroup.firstSlavePos = pos;
    fwrite(&masterGroup, sizeof(struct Group), 1, masterTable);

    printf("Successfully created with code #%d!", code);
    printf("\n");
}

struct getSReturn getS(){
    short found = 0;
    struct getMReturn masterRet; masterRet.found = 0;
    int grcode;

    struct Student badst = {0,0,"","",0,0, -1};
    struct getSReturn badret = {0, badst, -1, -1};

    while (masterRet.found == 0){
        printf("> Enter the group code or -1 to exit:");
        scanf("%u", &grcode);

        if(grcode == -1) return badret;
        masterRet = getM(grcode);
        if (masterRet.found == 0) printf("! Group not found !\n");
    }
    printf("Searching for a student from the group\n");
    printGroup(masterRet.group);
    if (masterRet.group.firstSlavePos == -1) {
        printf("! This group has no students !\n");
        return badret;
    }

    unsigned int code;
    printf("> Enter the code of the student:");
    scanf("%u", &code);

    int pos = masterRet.group.firstSlavePos;
    struct Student st;

    while (pos != -1){
        fseek(slaveTable, pos * sizeof(struct Student), SEEK_SET);
        fread(&st, sizeof(struct Student), 1, slaveTable);
        if(st.code == code) break;
        if(st.nextSlavePos == -1) {
            printf("! Not found !\n");
            return badret;
        }
        pos = st.nextSlavePos;
    }

    printStudent(st);
    struct getSReturn ret = {1, st, pos, masterRet.pos};
    return ret;
}

void updateS(){
    struct getSReturn stret = getS();
    if (stret.found == 0) return;
    struct Student st = stret.student;

    // get data
    printf("> Enter the new name of the student (previously - %s):", st.name);
    scanf("%s", st.name);

    struct getMReturn newMaster;
    newMaster.found = 0; int grcode;
    while (!newMaster.found){
        printf("> Enter the new group code (previously - %i):", st.groupCode);

        scanf("%i", &grcode);
        if(grcode == -1) return;
        if(grcode != st.groupCode){
            newMaster = getM(grcode);
            if(!newMaster.found){
                printf("! Group not found ! Enter -1 to leave\n");
                continue;
            } else break;

        } else break;
    }

    if(newMaster.found){
        // edit previous master or slaves
        struct Group oldMaster;
        fseek(masterTable, stret.masterPos * sizeof(struct Group), SEEK_SET);
        fread(&oldMaster, sizeof(struct Group), 1, masterTable);
        if(oldMaster.firstSlavePos == stret.pos){
            oldMaster.firstSlavePos = st.nextSlavePos;
            fseek(masterTable, stret.masterPos * sizeof(struct Group), SEEK_SET);
            fwrite(&oldMaster, sizeof(struct Group), 1, masterTable);
        } else {
            struct Student curSt;
            curSt.nextSlavePos = oldMaster.firstSlavePos;
            while (curSt.nextSlavePos != stret.pos){
                fseek(slaveTable, curSt.nextSlavePos * sizeof(struct Student), SEEK_SET);
                fread(&curSt, sizeof(struct Student), 1, slaveTable);
            }
            curSt.nextSlavePos = st.nextSlavePos;
            fseek(slaveTable, -1 * sizeof(struct Student), SEEK_CUR);
            fwrite(&curSt, sizeof(struct Student), 1, slaveTable);
        }

        // add to new master
        int newMasterFS = newMaster.group.firstSlavePos;
        newMaster.group.firstSlavePos = stret.pos;
        st.nextSlavePos = newMasterFS;
        st.groupCode = grcode;
        fseek(masterTable, newMaster.pos * sizeof(struct Group), SEEK_SET);
        fwrite(&newMaster.group, sizeof(struct Group), 1, masterTable);
    }

    printf("> Enter the new way of education funding (previously - %s):", st.educationFunding);
    scanf("%s", st.educationFunding);
    printf("> Enter the new birth year (previously - %i):", st.birthYear);
    scanf("%i", &st.birthYear);

    fseek(slaveTable, stret.pos * sizeof(struct Student), SEEK_SET);
    fwrite(&st, sizeof(struct Student), 1, slaveTable);

    printf("Successfully updated!\n");
}

void deleteS(){

    struct getSReturn stret = getS();
    if(!stret.found) return;

    printf("Are you sure you want to delete this record? (Y/N) ");
    char ans[50];
    scanf("%s", ans);
    if(strcmp(ans, "Y") != 0) return;

    // edit previous master or slaves
    struct Group master;
    fseek(masterTable, stret.masterPos * sizeof(struct Group), SEEK_SET);
    fread(&master, sizeof(struct Group), 1, masterTable);
    if(master.firstSlavePos == stret.pos){
        master.firstSlavePos = stret.student.nextSlavePos;
        fseek(masterTable, stret.masterPos * sizeof(struct Group), SEEK_SET);
        fwrite(&master, sizeof(struct Group), 1, masterTable);
    } else {
        struct Student curSt;
        curSt.nextSlavePos = master.firstSlavePos;
        while (curSt.nextSlavePos != stret.pos){
            fseek(slaveTable, curSt.nextSlavePos * sizeof(struct Student), SEEK_SET);
            fread(&curSt, sizeof(struct Student), 1, slaveTable);
        }
        curSt.nextSlavePos = stret.student.nextSlavePos;
        fseek(slaveTable, -1 * sizeof(struct Student), SEEK_CUR);
        fwrite(&curSt, sizeof(struct Student), 1, slaveTable);
    }

    addToStack(&slaveTrashIndexes, stret.pos);
    stret.student.phys = 0; stret.student.nextSlavePos = -1;
    fseek(slaveTable, stret.pos * sizeof(struct Student), SEEK_SET);
    fwrite(&stret.student, sizeof(struct Student), 1, slaveTable);

    printf("Successfully deleted.\n");
}


void utilMS(){
    printf("> Enter the code of the group:");
    unsigned int code;
    scanf("%u", &code);
    struct getMReturn ret = getM(code);
    if (ret.found == 0){
        printf("! Record not found !\n");
        return;
    }

    printf("Students of the group\n");
    printGroup(ret.group);
    printf("are:\n");

    struct Student st; int c = 0;
    st.nextSlavePos = ret.group.firstSlavePos;
    while (st.nextSlavePos != -1){
        fseek(slaveTable, st.nextSlavePos * sizeof(struct Student), SEEK_SET);
        fread(&st, sizeof(struct Student), 1, slaveTable);
        printStudent(st); c++;
    }

    if(c==0) printf("! No students found!\n");
    else printf("Total: %d students\n", c);
}

void addToStack(struct Stack *stack, int val){
    struct StackNode *newPtr = malloc(sizeof(struct StackNode));
    newPtr->val = val;
    newPtr->next = stack->top;
    stack->top = newPtr;
    stack->len++;
}
int popStack(struct Stack *stack){
    int ret = stack->top->val;
    stack->len--;
    stack->top = stack->top->next;
    return ret;
}

void readTrashToStack(char filename[], struct Stack *stack){
    FILE* trashFile = fopen(filename, "rb");
    if(!trashFile) return;
    long len = fileLen(trashFile);
    while (ftell(trashFile) < len) {
        int temp;
        fread(&temp, sizeof(int), 1, trashFile);
        addToStack(stack, temp);
    }
    fclose(trashFile);
}

void writeTrashToFile(char filename[], struct Stack *stack){
    FILE* trashFile = fopen(filename, "wb");
    if(!trashFile) { printf("Couldn't write trash!\n"); return; }
    while (stack->len > 0) {
        int temp = popStack(stack);
        fwrite(&temp, sizeof(int), 1, trashFile);
    }
    fclose(trashFile);
}