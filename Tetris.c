#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/*ver1.0 @2022/01/29
これはテトリスゲームです。

【注意】
810行目printBufferAdd(center_display_strings[h]);
の実行を堺にsystem("cls")で落ちるようになる。原因は不明。
よって画面クリアは行わない。
適切な高さにウインドウを調整すれば同じ位置に表示されるはず
(それまでに表示されていたコンソールの内容が消されます。)
ローカルマシンで動かすことを推奨

【テトリス】
コンソールでテトリスができます。
下に説明するキーを押してEnterを押すことで操作ができます。
操作をするまで盤面は動かず、制限時間もありません。
＜操作キー＞
A:左
D:右
S:下
W:ハードドロップ（一番下まで一気に落ちる）
Q:左回転
E:右回転
R:ホールドと交換
C:終了

得点は処理したテトリミノの数、および所要時間
スコアは生成したテトリミノの数
ホールドが元々入ってるのは仕様

【プログラム】
概要
テトリミノなどの定義
文字列をコンソールに出力する部分
出力する文字列を生成する部分
フィールドの状態管理配列
キー入力操作
から成り立っている
*/
//これをコメントアウトすることで本番モードになる
//#define is_debug 1
/*
並び順、本来ならこの分量はページを分けたいが提出のために一枚にした
・デバックマクロ
・型の定義（テトリミノの形状、角度、色）
    テトリミノは空白含め2*4の形状だが後述の共通化のために要素数８の一次元配列にフラット化した
    ４つの角度それぞれに縦横の長さと、出力する順番配列を定義しておき、
    出力順の配列を間に入れることで全てのテトリミノ、角度について共通のアルゴリズム
    を使うことができる。
・コンソール出力制御（print buffer）
    printfは時間がかかるので全ての文字列を結合してから一回で出力する
・OS設定（未使用）
    OSに依存する機能（非同期キー読み取り、スレッド）は使用しないことにしたので使われていない
・ゲームフィールド変数定義
    場の二次元配列、現在持っているテトリミノの種類と角度など。
・描画指示文の作成系
    画面出力の文字列を作成する。左、テトリス盤面、右の３つの二次元配列[高さ、行][文字列]で作成して、最後に合成する。
    centerのカラムはゲームフィールドの配列に格納されている色数字（1,2,3,...）を読み出して
    色を変更する文字列を添付して"  "を出力している。
    既に場に固定された色と現在持っているテトリミノの情報を合成したあらたな数字の配列を作って
    それを元に出力文字列を作成する。
・ゲームロジック
    キー入力、操作適用、衝突判定、底についた判定、新規生成、ビンゴ行の除去
    ここでは数字の盤面配列の操作を主にする。あと今持っているものとホールドのもの
    描画指示もここが出している：displayShow()
・ライフサイクル
    onCreate(),onGameStart(),onDestroy()の３つがある。
    継承とかしないしべつに分ける必要はないが、わかりやすくするため。
・デバック（主にテトリミノ描画）
    テトリミノ単体を出力したり、角度付ける配列システムを検証するコード
・main()
    ３つのライフサイクルがあるだけ。あとはコメントアウトされたデバック関数たち。
*/




//ーー調整系ーーーーーーーーーーーーーーーーー
/*
//最初の強制移動までの時間、秒
#define INTERVAL_TIME 3
//一定時間に処理すべきテトリミノの数
#define DEFAULT_ACTION_TO_DO 2
//このスコアを得るごとに行うべきアクションが+1されていく
#define SCORE_INTERVAL_TO_CHANGE_STAGE 15
*/


//デバック？
//本番ならこれをコメントアウト
   //#define is_debug 1

//#define debug_printBuffer 1

//ーーデバックマクローーーーーーーーーーーーー

/*本番での処理に影響しないようにデバック関係を取り除ける*/


#ifdef is_debug
    //printf("debug:{str}\n");
    #define debug(str) \
        do{\
            printf("debug:");\
            printf(str);\
            printf("\n");\
        }while(0);
    
    //debug_icast("hoge%d",int) => "debug:hoge{int}\n"
    #define debug_icast(str,int) \
        do{\
            printf("debug:");\
            printf(str,int);\
            printf("\n");\
        }while(0);

    //debug_scast("hoge%s", char* str) => "debug:hoge{string}\n"
    #define debug_scast(str, s)\
        do{\
            printf("debug:");\
            printf(str,s);\
            printf("\n");\
        }while(0);
    
#else
    #define debug(str) {/*何もしない*/}
    #define debug_icast(str,int) {}
    #define debug_scast(str,s) {}
#endif

/*<stdlib.h>*/
int fatailError(char *str){
    printf("Error>>");
    printf(str);
    printf("\nprogram stoped\n");

    exit(1);
}

//ーー変数および型の定義ーーーーーーーーーーーーーーー

//フィールドの大きさ
#define WIDTH 10
#define HEIGHT 20

/*図形をそれぞれ定義するエリア*/
struct tetromino {
    //int body[2][4];
    //一次元にフラット化した。このほうが回転させる出力データを書きやすい
    //2*4の場所に連番IDつけた感じ
    int body[8];
    };
typedef struct tetromino Tetromino;
//0 is null. 1 is blue. 2 is yellow. 3 is oreange. 4 isgreen. 5 is pink.
const Tetromino I = {//青い横長
    //最初の４つが上段、残り４つが下段
    {1,1,1,1,0,0,0,0}//1は青
};
const Tetromino O = {//黄色い四角いの
    {0,2,2,0,0,2,2,0}//できるだけ真ん中に寄せる
};
const Tetromino L = {//オレンジL型
    {3,3,3,0,3,0,0,0}
};
//L_reverse
const Tetromino Lr ={
    {3,3,3,0,0,0,3,0}
};
const Tetromino S = {//緑の雷型
    {4,4,0,0,0,4,4,0}
};
const Tetromino Sr = {
    {0,4,4,0,4,4,0,0}
};
const Tetromino T = {//赤の短いT
    {5,5,5,0,0,5,0,0}
};

//まとめて扱うために配列化。ランダム数字とTetを結びつける
const Tetromino * const tet_list[7] = {&I, &O, &L, &Lr, &S, &Sr, &T};
//IDじゃなくてポインタで管理できるな。Cだとそういうやり方があるのか。まあもう書いちゃったし。
typedef enum {
    I_id = 0,//これ以降1,2,3,と割り振られる
    O_id,
    L_id,
    Lr_id,
    S_id,
    Sr_id,
    T_id,
}TetID;
//名前からの呼び出し用。手書きのため。
typedef struct {
    const  Tetromino *i,*o, *l, *l_r, *s, *s_r, *t;
}Tets;
const Tets Tet = {&I, &O, &L, &Lr, &S, &Sr, &T}; 



//回転角度
typedef enum {
    arg0_id,
    arg90_id,
    arg180_id,
    arg270_id
} ArgID;
typedef struct {
    int height;
    int width;
    //回転状態への変換出力用
    //上の横幅で改行しつつこれの中身を前から一個ずつ出力していく。
    int print_order[8];
    //回転したら次どの角度になるか。int足し引きはバグ生みそう。４種類なら明記したほうが早い
    ArgID rotate_left_id;
    ArgID rotate_right_id;
} ArgData;
/*ID配置
0 1 2 3
4 5 6 7
*/
//IDを高さと横幅決めた上でこの順番で出せばよい。
//これによって条件分岐が１つも要らなくなる。
const ArgData arg0 = {
    2, 4,
    {0,1,2,3,  4,5,6,7},//普通の
    arg90_id, arg270_id
};
const ArgData arg90 = {
    4,2,
    {3,7,  2,6,  1,5,  0,4},
    arg180_id, arg0_id
};
const ArgData arg180 = {
    2,4,
    {7,6,5,4,  3,2,1,0},
    arg270_id, arg90_id
};
const ArgData arg270 = {
    4,2,
    {4,0,  5,1,  6,2,  7,3},
    arg0_id, arg180_id
};
//ポインタなのは「式には定数値が必要です」のため
//ArgIDで取る
const ArgData *arg_data_list[4] = {&arg0, &arg90, &arg180, &arg270};



//色。これを文字列の前につけることでそれ以降の色が変わる
//color_list[id]で色のコードを取り出せる
const char color_list[6][9] = {"\x1b[49m","\x1b[46m","\x1b[43m","\x1b[41m","\x1b[42m","\x1b[45m"};
//取り出し用のID
typedef enum {
    nocolor_id = 0,
    cyan_id,
    yellow_id,
    orange_id,
    green_id,
    mazenta_id
}ColorID;
//名前から直接文字列出したいとき
typedef struct {
    char none[9];
    char cyan[9];
    char yellow[9];
    char orange[9];
    char green[9];
    char mazenta[9];
} ColorStrs;
//とりあえずオレンジは赤で代用
const ColorStrs ColorStr = {"\x1b[49m","\x1b[46m","\x1b[43m","\x1b[41m","\x1b[42m","\x1b[45m"};







//ーー描画系ーーーーーーーーーーーーーーーーーーーーーーーーーー
//mallocならスコープないからいらないのでは
//→いちいち引数でポインタ渡さなくて済む。
char *print_buffer_p;
//const int number_of_full_width;
int is_windows = 0;//winのとき１
int is_linux = 0;//linuxのとき１

//描画範囲＋２０００くらいあらばいいのでは。UIつけるときは要注意だな
const int print_buffer_size = 2000;
int print_buffer_used = 0;

int clearPrintBuffer(){
        debug("clearPrintBuffer()");
        for(int i = 0;i < print_buffer_size;i++){
            print_buffer_p[i] = '\0';
        }
        //使った分をクリア
        print_buffer_used = 0;
        return 0;
    }

//__free__の関数は戻り値にfreeが必要
/*Rquirement
const int print_buffer_size*/
char* setPrintBuffer(){
    debug("-start- setPrintBuffer()");
    

    print_buffer_p = (char*)malloc(print_buffer_size * sizeof(char));
    if(print_buffer_p == NULL){
        perror("Error>>failed to setPrintBuffer__free():");
        //異常終了
        exit(1);
    }
    debug_icast("created print buffer size is [%d]",print_buffer_size);
    clearPrintBuffer();
    return print_buffer_p;
}
void freePrintBuffer(){
    free(print_buffer_p);
}

/*Requirement
<string.h>
const int print_buffer_size
int print_buffer_used*/
int printBufferAdd(const char *to_add){
    #ifdef debug_printBuffer
    debug("printBufferAdd()");
    #endif

    #ifdef is_debug
    if(print_buffer_p == NULL){
        fatailError("print buffer is not initialized. You must call setPrintBuffer() before use buffer.");
    }
    #endif

    int add_size = sizeof(to_add);
    if(print_buffer_used + add_size < print_buffer_size){
        //ここで追加
        strcat(print_buffer_p, to_add);
        //追加分を記録
        print_buffer_used += add_size;
        #ifdef debug_printBuffer
        debug_icast("buffer added. now print_buffer_used is = [%d]", print_buffer_used);
        #endif
    }else{
        printf("buffer size is not enough.\n");
        printf("buffer size = [%d], buffer used = [%d], add size = [%d]\n",print_buffer_size, print_buffer_used, add_size);
        fatailError("failed to add text into print buffer.");
    }
    
    
    return 0;
}

//OSによって対応を変える
/*Reqirement
<stdlib.h>
int is_windows;
int is_linux;
fatailError(),<stdlib.h>
*/
int consoleClear(){
    /*対処不能なためスキップ
    if(is_windows){
        debug("consoleClear in is_windows befre cls");
        //system("date");//これは動く。
        system("cls");
        debug("consoleClear in is_windows after cls");
        //system("cls");//これは動作する
    }else if(is_linux){
        system("clear");
    }else{
        fatailError("os type is not defined. you must call setOsType() before.");
    }*/
    return 0;
}




int writePrintBuffer(){
    debug("-start- writePrintBuffer()");
    
    //バッファのテキストを一気に出力
    printf("%s", print_buffer_p);
    //出力したので消す
    clearPrintBuffer();

    #ifdef is_debug
    //デバック情報が消えないようにする
    #else
    consoleClear();
    #endif
    consoleClear();

    return 0;
}




/*ーーーOS設定ーーーーーーーーーーーーーーーーーーーーーーーーーー*/
int setOsType(int os_type){
    if(os_type == 0){
        debug("set os type as windows");
        is_windows = 1;
        is_linux = 0;
        return 0;
    }else if(os_type == 1){
        debug("set os type as linux");
        is_windows = 0;
        is_linux = 1;
        return 0;
    }else{
        fatailError("os_type is invalid number.");
    }
    return 1;
}

int scanfInt(){
    int i;
    int result = scanf("%d", &i);//途中で止めた次の実行とかで止まらずに無限ループ
    if(result != 1){
        fatailError("failed to scanf int");
    }
    return i;
}

int inputOsType(){
    int os_type = -1;
    while(1){
        printf("Select OS type which this program runs on.\nwindows : 0, linux : 1\n:");
        
        os_type = scanfInt();
        
        debug("input os_type loop");
        if(os_type == 0){
            debug("windows");
            is_windows = 1;
            is_linux = 0;
            break;
        }else if(os_type == 1){
            debug("linux");
            is_windows = 0;
            is_linux = 1;
            break;
        }else{
            printf("input [%d] is invalid number. Input again.\n", os_type);
        }
    }
    return os_type;
}
//ーーゲームのフィールド変数ーーーーーーーーーーーー
//得点4桁まで
int score = 0;
time_t start_time;
int esped_time = 0;
//
//int action_to_do_in_interval = DEFAULT_ACTION_TO_DO;
//int nokori_action = 0;

//二次元座標型を作る
typedef struct {
    int x, y;
    } Point;
//Tetの回転中心の定義
const Point Tet_origin = {0 ,0};
//新規生成の位置
const Point default_spawn_point = {4, 0};
//！！！field[y][x]であることに注意！


//これは数字が入ってる
//0 is null. 1 is blue. 2 is yellow. 3 is oreange. 4 isgreen. 5 is pink.
int game_logic_field[HEIGHT][WIDTH] = {0};
//上のにcurrentなテトリミノのレイヤーを合成したもの。これをもとに描画文字列を作る
int color_field_to_show[HEIGHT][WIDTH] = {0};

//別々に文字列化した後に合成する
//centerはテトリスの文字盤
//色の指示があるのでとりあえず10倍確保
char center_display_strings[HEIGHT][WIDTH*10] = {"never initialized!"};
char left_display_strings[HEIGHT][WIDTH*10];
char right_display_strings[HEIGHT][WIDTH*10];

//現在のTet
Tetromino current_tet;
ArgID current_arg_id;
//Tet原点のグローバル座標　Max(10, 20)
Point current_point = {4,0};

Tetromino hold_tet;
ArgID hold_arg_id;

TetID next_tet_id[5];

//ーーー描画指示文字列作成ーーー
void clearAllDisplayStrings(){
    debug("--start-- clearAllDisplayStrings()");
    //これを毎フレームごとに呼ばないと色指示の長さ違いで前のが残る
    
    //各種配列の０化
    int w = 0;
    //高さは共通化できる
    for(int h = 0;h < HEIGHT;h++){
        ////WIDTH*2!　*10にしてたから関係ない変数巻き込んでバグってた
        //いや、10倍まで確保してるはず。あれれ？？
        for(w = 0;w < WIDTH*10-1;w++){
            //表示の初期化
            center_display_strings[h][w] = '\0';
            left_display_strings[h][w] = '\0';
            right_display_strings[h][w] = '\0';
        }
    }
}

void clearDisplayStrings(char target[HEIGHT][WIDTH*10]){
    debug("--start-- clearDisplayStrings()");
    int w = 0;
    //高さは共通化できる
    for(int h = 0;h < HEIGHT;h++){
        //WIDTH*2!　*10にしてたから関係ない変数巻き込んでバグってた
        for(w = 0;w < WIDTH*10-1;w++){
            //表示の初期化
            target[h][w] = '\0';
        }
    }
}


void initializeField(){
    debug("--start-- initializeField()");

    //盤面の０化。{0}でできるみたいで、いらんが一応
    int w = 0;
    for(int h = 0;h < HEIGHT;h++){
        for(w = 0;w < WIDTH;w++){
            //ゲーム盤の初期化
            game_logic_field[h][w] = 0;   
        }
    }

    //文字列合成用の３カラムをクリア
    clearAllDisplayStrings();


    //式には定数値が必要ですって言われちゃうのでここで初期化
    //nullはこわいのでとりあえずなんか入れる
    current_tet = *Tet.i;
    current_arg_id = arg0_id;

    hold_tet = *Tet.o;
    hold_arg_id = arg0_id;

    next_tet_id[0] = L_id;
    next_tet_id[1] = Lr_id;
    next_tet_id[2] = S_id;
    next_tet_id[3] = Sr_id;
    next_tet_id[4] = T_id;
}



// |01234567890123456789|を担当するカラム
void setCenterDisplayStrings(){
    debug("--start-- setCenterDisplayStrings()");
    //debug_icast("next_tet_id[0] is %d @setCenterDisplayStrings", next_tet_id[0]);

    clearDisplayStrings(center_display_strings);

    for(int h = 0;h < HEIGHT;h++){
        //仕切り|はcenterの担当
        
        //合成するときに付ける
        //パフォーマンス的には悪いがサイドは単純じゃないのでコードが複雑に
        //strcat(center_display_strings[h], "|");

        for(int w = 0;w < WIDTH;w++){
            //色の読み出し
            ColorID this_color_id = color_field_to_show[h][w];
            const char *const color_string = color_list[this_color_id];
            //色の指示文を格納
            strcat(center_display_strings[h], color_string);
            strcat(center_display_strings[h], "  ");
        }
        //次の行に影響しないように
        strcat(center_display_strings[h], ColorStr.none);

        //右の仕切り
        //strcat(center_display_strings[h], "|");
    }


}

int addTetriminoToDisplayStrings(char targetDispayStrings[HEIGHT][WIDTH*10],int h_to_write, Tetromino tet, ArgID arg_id){
    debug("--start-- addTetrimonoToDisplayStrings()");
    //debug_icast("next_tet_id[0] is %d", next_tet_id[0]);


    //回転０の情報を受取る
    const int tet_height = arg_data_list[arg_id]->height;
    const int tet_width = arg_data_list[arg_id]->width;
    const int *order = arg_data_list[arg_id]->print_order;

    int tet_arg_place_num = 0;
    //debug_icast("next_tet_id[0] is %d", next_tet_id[0]);
    //4*4で埋めたいので横向きのときは上下に一行ずつ
    if(tet_height == 2){
        //debug("1");
        //printf("%d", h_to_write);
        
        //printf("<%s>", targetDispayStrings[h_to_write]);
        //printf("<%s>", left_display_strings[h_to_write]);
        
        /*引数をポインタにしてたからここで止まってた。配列はポインタにしたら性質が失われる*/
        //上にい１行, " "を8個
        strcat(targetDispayStrings[h_to_write], "        ");

        //debug_icast("next_tet_id[0] is %d", next_tet_id[0]);
        //debug("2");
        //スタートを一個ずらす
        h_to_write++;
    }
    //debug_icast("next_tet_id[0] is %d", next_tet_id[0]);
    //debug("a");
    //h_to_write行目から書き込み
    //(h,w) = (2, 4) or (4, 2)
    for(int h_temp = 0;h_temp < tet_height;h_temp++){
        //strcat(targetDispayStrings[h_to_write], "d");

        //横幅を4にするため
        if(tet_width == 2){
            //左に１つ文空白
            strcat(targetDispayStrings[h_to_write], "  ");
        }

        //テトリミノの部分
        for(int w = 0;w < tet_width;w++){
            //その場所に書くべき色を読み出す
            const int tet_orignal_place_num = order[tet_arg_place_num];
            const ColorID this_point_color_id = tet.body[tet_orignal_place_num];
            
            //printf("h to write is :%d\n",h_to_write);
            //strcat(targetDispayStrings[h_to_write], "e");

            //h行目に色指示文字列を追加
            strcat(targetDispayStrings[h_to_write], color_list[this_point_color_id]);
            //上の色で１マス追加
            strcat(targetDispayStrings[h_to_write], "  ");

            tet_arg_place_num++;   
        }

        //これかーーーー！いや、違うみたいだ。あれ、今なんかした？動くからヨシ？
        //次の行に影響しないように
        strcat(targetDispayStrings[h_to_write], ColorStr.none);

        //横幅を4にするため
        if(tet_width == 2){
            //左に１つ文空白
            strcat(targetDispayStrings[h_to_write], "  ");
        }
        h_to_write++;
    }

    //横のとき
    if(tet_height == 2){
        //下に１行, " "を8個
        strcat(targetDispayStrings[h_to_write], "        ");
        //スタートを一個ずらす
        h_to_write++;
    }
    //debug_icast("next_tet_id[0] is %d", next_tet_id[0]);
    //これを書き込んだ後のために次に書き込むべき空白の行を返す
    return h_to_write;
}

//左側半角8文字
void setLeftDisplayStrings(){
    debug("--start-- setLeftDisplayStrings()");

    clearDisplayStrings(left_display_strings);

    //このカラムの次に書き込むべき行
    int h_next = 0;

    
    //holdのテトリミノ
    h_next = addTetriminoToDisplayStrings(left_display_strings, h_next, hold_tet, hold_arg_id);
    
    char *text_color_red = "\x1b[32m";
    char *text_color_none = "\x1b[37m";
    //得点
    strcat(left_display_strings[h_next], "SCORE is");
    h_next++;
    //score
    char temp[20];
    sprintf(temp, "%snow:%04d%s", text_color_red, score, text_color_none);
    strcat(left_display_strings[h_next], temp);
    h_next++;
    
    //現在の所要時間
    strcat(left_display_strings[h_next], "@elapsed");
    h_next++;
    sprintf(temp, "%s%05dsec%s", text_color_red, esped_time, text_color_none);
    strcat(left_display_strings[h_next], temp);
    h_next++;
    
    
    
    #define msg_num 12
    char msg[msg_num][9] = {
        "        ",
        "key cfg ",
        "A:left  ",
        "D:right ",
        "S:down  ",
        "W:drop  ",
        "Q:rotate",
        "  left  ",
        "E:rotate",
        "  right ",
        "R:hold  ",
        "C:<EXIT>"
    };
    for(int i = 0;i < msg_num;i++){
        strcat(left_display_strings[h_next], msg[i]);
        h_next++;
    }

    /*
    text 1
    hold 4
    score 2
    msg 13
    合計　20。20入れる必要がある
    
    */
    
}

//右側半角8文字
void setRightDisplayStrings(){
    debug("--start-- setRightDisplayStrings()");

    clearDisplayStrings(right_display_strings);
    //debug_icast("next_tet_id[0] is %d @setRightDisplayStrings", next_tet_id[0]);

    int h_to_write = 0;
    char msg[9];
    for(int i = 0;i < 4;i++){
        debug("1");
        //何番目のものかラベルを表示
        sprintf(msg, " < %1d >  ", i);
        strcat(right_display_strings[h_to_write], msg);
        h_to_write++;
        debug("1");

        //次のテトリミノリストからi番目のテトリミノのIDを出してそのIDのテトリミノを描画
        TetID tet_id = next_tet_id[i];//まだ次のIDリストができてないのに読み込んでたからだ
        //なんかここで止まるな（２回目）、テトリミノが一番上まで来たときだからそのへんでオーバーランしてるか？
        //printf("next_tet_id[%d] is %d", i, next_tet_id[1]);
        //printf("%d", tet_id);//変な数
        Tetromino tet = *tet_list[tet_id];
        debug_icast("h_to_write is %d", h_to_write);
        h_to_write = addTetriminoToDisplayStrings(right_display_strings, h_to_write, tet, arg0_id);
        //debug_icast("h_to_write is %d", h_to_write);
    }
    /*
    msg 1
    tet 4
    が五回だからちょうど20
    */
}

/*
logicの盤面とその他のテトロミノ情報から縦ごとに
right center leftの３つのカラム、DisplyStringsを作り
それをPrintBufferにまとめて一気に書き出す

*/

void setAllDisplayStrings(){
    debug("--start-- setAllDisplayStrings()");

    debug_icast("next_tet_id[0] is %d @setAllDisplayStrings", next_tet_id[0]);//正常
    setLeftDisplayStrings();
    setCenterDisplayStrings();
    setRightDisplayStrings();
}

void makePrintBufferFromDisplayString(){
    debug("--start-- makePrintBufferFromDisplayString()");
    clearPrintBuffer();

    //ヘッダー
    printBufferAdd("----------------------------------------\n");
    char temp[50] = {'\0'};
    sprintf(temp, " <Hold>  Play Tetris on Console!  <Next>\n");
    printBufferAdd(temp);
    
    //本体
    for(int h = 0;h < HEIGHT;h++){
        printBufferAdd(left_display_strings[h]);
        printBufferAdd("||");//printf("piyo1");system("cls");printf("piyo3");
        printBufferAdd(center_display_strings[h]);
        printBufferAdd("|| ");//printf("piyo1 %d", h);system("cls");printf("piyo4");
        printBufferAdd(right_display_strings[h]);
        printBufferAdd("\n");//printf("piyo1");system("cls");printf("piyo5");
        //うーん。ここの17回目の実行でsystem("cls")で止まる。理由はなんだ？？
        //17回目、特殊なのはなんだ？左側か？
        //どうもcenterのところらしい
        /*for(int i = 0;i < WIDTH*10-2;i++){
            if(center_display_strings[h][i] != center_display_strings[h+1][i]){
                printf("deffernt!");
            }
            printf("%c,", center_display_strings[h][i]);
        }printf("%d\n",h);*/
        /*printf("<%s>\n", left_display_strings[h]);
        printf("<%s>\n", center_display_strings[h]);
        printf("<%s>\n", right_display_strings[h]);
        printf("piyo1 %d\n",h);system("cls");printf("piyo2 %d\n",h);
        */
    }
    
    //フッター
    printBufferAdd("----------------------------------------\n");

    #ifdef is_debug
    
    printf("left_display_strings is\n");
    for(int i = 0;i < 20;i++){
        printf("row %d is <%s>\n", i, left_display_strings[i]);
    }
    printf("center_display_string[0] is <%s>\n", center_display_strings[0]);
    
    //なんか最初から２つはいってる問題調査用
    printf("game_logic_field[0] is\n");
    for(int i = 0;i < WIDTH;i++){
        printf("%d", game_logic_field[0][i]);
    }printf("\n");
    
    
    
    printf("right_display_string[0] is <%s>\n", right_display_strings[0]);
    debug("--fin-- makePrintBufferFromDisplayString()")
    #endif

    
}

//ーーーゲームロジックーーー




typedef enum {
    a = 'a',
    s = 's',
    d = 'd',
    e = 'e',
    w = 'w',
    q = 'q',
    r = 'r',
    c = 'c'
}Input;

typedef enum {
    ok = 1,
    yes = 1,
    no = 0,
}Result;

/*
Result hold_tet_chaned = no;
Result stage_changed = no;
Result next_tet_list_chaged = no;
*/
//exit の cキーが押されたことの検出
Result continue_game = yes;

//-----------------------------------
int getRandom(int min, int max){
  //rand()*(最大値－最小値　+1.0)/(1.0+RAND_MAX)+minすることで最小値から最大値までの範囲の乱数を得る
  //+1はintが切り捨てだから？
  //全体の値かける０から１の倍率で決めている
  int random = rand()*(max-min+1.0)/(1.0+RAND_MAX)+min; 

  return random;
}

TetID getRandomTetID(){
    //tet は７種類の０始まりenum
    TetID tet_id = (TetID)getRandom(0, 6);
    return tet_id;
}

void initializeNextTetIDList(){
    debug("--start-- initializeNextTetIDList()");
    //5はnext_tet_idの要素数
    for(int i = 0;i < 5;i++){
        next_tet_id[i] = getRandomTetID();
        debug_icast("next_tet_id[i] is :%d", next_tet_id[i]);
    }
}

void __updateNextTetIDList(){
    debug("__updateNextTetIDList()");
    //ずらす
    //0番目はcurrentに出ていくので消してよし
    for(int i = 0;i < 4;i++){
        next_tet_id[i] = next_tet_id[i+1];
    }
    //ランダムに取得して最後に追加
    next_tet_id[4] = getRandomTetID();
}
//spawnNewTet()で使いたいので。プロトタイプ宣言
Result __checkCollision(Point point_move_to, ArgID arg_move_to);

Result spawnNewTet(){
    debug("spawnNewTet()");
    //新しいTetをセット
    TetID new_tet_id = next_tet_id[0];
    current_tet = *tet_list[new_tet_id];
    //「次の」リスト更新
    __updateNextTetIDList();

    //新規のは角度０
    current_arg_id = arg0_id;

    //生成位置
    //フィールドが埋まってきてデフォルトの生成点だと衝突するかもしれない。
    //5は横幅の半分
    int shift = 0;
    for(int w = 0;w < 5;w++){
        debug_icast("search available spawn point at shift = %d", shift);
        //できるだけ中心部で生成する
        //左シフト
        Point spawn_point = {4 - shift, 0};
        if(__checkCollision(spawn_point, arg0_id)){
            debug_icast("find spawn point at w = %d", (4-shift));
            //okならそこに指定して終了
            current_point.x = spawn_point.x;
            current_point.y = spawn_point.y;
            return ok;
        }
        //右シフト
        Point spawn_point2 = {5 + shift, 0};
        if(__checkCollision(spawn_point2, arg0_id)){
            debug_icast("find spawn point at w = %d", (shift +5));
            current_point.x = spawn_point2.x;
            current_point.y = spawn_point2.y;
            return ok;
        }
        shift++;
    }
    //上のforでどこも成功しなかったら生成失敗
    return no;
}

//-----gameMain()のエリア-------------------------------



Result __checkCollision(Point point_move_to, ArgID arg_move_to){
    debug("__checkCollision()");
    //(this_x, this_y)の点についてそれぞれ調べる
    const int tet_height = arg_data_list[arg_move_to]->height;
    const int tet_width = arg_data_list[arg_move_to]->width;
    const int *order = arg_data_list[arg_move_to]->print_order;

    int this_x, this_y;
    int tet_arg_place_num = 0;
    for(int h = 0;h < tet_height;h++){
        this_y = point_move_to.y + h;
        for(int w = 0;w < tet_width;w++){
            this_x = point_move_to.x + w;
            
            //(this_x, this_y)に表示すべきtetの色
            int tet_original_place_num = order[tet_arg_place_num];            
            int this_tet_color_id = current_tet.body[tet_original_place_num];
            
            if(this_tet_color_id != 0){
                //Tetに実体があるとき、座標が場外ならアウト
                if(this_x < 0 || (WIDTH-1) < this_x || this_y < 0 || (HEIGHT-1) < this_y){
                    debug("-> collide! because out of field.");
                    return no;

                //Tetに実体があるとき、背景に実体があると衝突
                }else if(game_logic_field[this_y][this_x] != 0){
                    debug("-> collide! because conflict with game_logic_field");
                    return no;
                }
            }
             
            //次のマスを検証
            tet_arg_place_num++;
        }
    }
    debug("-> return ok. no collision conflict or out of field.");
    //どこにも引っかからなけれOK
    return ok;
}

Point __findGoodPointToBeRotated(ArgID arg_move_to){
    debug("__findGoodPointToBeRotated()");
    //左、左下、下、右下、右ずらしの中から最も距離が近い、ぶつからない位置を探す
    int shift = 1;
    while(1){
        //左、下、右を試す
        //左
        //{}できるのは宣言のときだけ
        Point point_move_to = {current_point.x - shift, current_point.y};
        if(__checkCollision(point_move_to, arg_move_to)){
            //見つけたポイントを返す
            return point_move_to;
        }
        //右
        Point point_move_to1 = {current_point.x + shift, current_point.y};
        if(__checkCollision(point_move_to, arg_move_to)){
            return point_move_to;
        }
        //下
        Point point_move_to2 = {current_point.x, current_point.y + shift};
        if(__checkCollision(point_move_to2, arg_move_to)){
            return point_move_to2;
        }

        //左下、右下は、２方向の移動なので同じshiftなら優先度が低い
        //左下
        Point point_move_to3 = {current_point.x - shift, current_point.y + shift};
        if(__checkCollision(point_move_to3, arg_move_to)){
            return point_move_to3;
        }
        //右下
        Point point_move_to4 = {current_point.x + shift, current_point.y + shift};
        if(__checkCollision(point_move_to4, arg_move_to)){
            return point_move_to4;
        }
        //2個以上ずらして再度検証
        shift++;
    }
}

Result __updateCurrentTetState(Input key){
    debug("__updateCurrentTetState()");
    switch (key){
        case a://左
        {//case x:の直後は式を要求していて宣言はだめらしい。よって{}ブロックをするか;で空の式を作るかする
            //Tetの原点座標を1左に
            Point point_move_to = {current_point.x -1, current_point.y};
            //次の位置がぶつからないか確認。角度は変化しない
            if(__checkCollision(point_move_to, current_arg_id)){
                //okなら値を上書き
                current_point.x = point_move_to.x;
                current_point.y = point_move_to.y;
            }
        }
            break;
        case d://右
        ;//対策 a label can only be part of a statement and a declaration is not a statement
            Point point_move_to2 = {current_point.x +1, current_point.y};
            if(__checkCollision(point_move_to2, current_arg_id)){
                current_point.x = point_move_to2.x;
                current_point.y = point_move_to2.y;
            }
            break;
        case s://下
        ;//対策 a label can only be part of a statement and a declaration is not a statement
            Point point_move_to3 = {current_point.x, current_point.y +1};
            if(__checkCollision(point_move_to3, current_arg_id)){
                current_point.x = point_move_to3.x;
                current_point.y = point_move_to3.y;
            }
            break;
        case e://右90度
        ;//対策 a label can only be part of a statement and a declaration is not a statement
            //->は言葉にするなら「このポインタの中の」
            ArgID arg_id_to_move = arg_data_list[current_arg_id]->rotate_right_id;
            //見つけたポイントが帰ってくるのでそれを現在のに代入
            current_point = __findGoodPointToBeRotated(arg_id_to_move);
            //回転も明示的に更新
            current_arg_id = arg_id_to_move;
            break;
        case q://左90度
        ;//対策 a label can only be part of a statement and a declaration is not a statement
            ArgID arg_id_to_move2 = arg_data_list[current_arg_id]->rotate_left_id;
            current_point = __findGoodPointToBeRotated(arg_id_to_move2);
            current_arg_id = arg_id_to_move2;
            break;
        case w://一番下まで落ちる
        ;//対策 a label can only be part of a statement and a declaration is not a statement
            Result is_ok = yes;
            Point point_move_to = current_point;
            while (is_ok){
                //１下げる。（座標は+）
                point_move_to.y += 1;
                is_ok = __checkCollision(point_move_to, current_arg_id);
            }
            //初めてnoが帰ってきた座標が記録されているのでその一個前が求めるもの
            point_move_to.y -= 1;
            current_point.x = point_move_to.x;
            current_point.y = point_move_to.y;
            break;
        case r://ホールドと交換
        ;//対策 a label can only be part of a statement and a declaration is not a statement
            Tetromino temp_tet = current_tet;
            ArgID temp_arg = current_arg_id;
            current_tet = hold_tet;
            current_arg_id = hold_arg_id;
            hold_tet = temp_tet;
            hold_arg_id = temp_arg;
            break;
        case c://ゲーム終了
            printf("%sQuit Game...%s\n", ColorStr.orange, ColorStr.none);
            continue_game = no;
            break;
    default:
        printf("You can't use the key '%c'", key);
        //無効なキーのときは０で判定不可を返す
        return no;
    }
    //1 means ok
    return ok;
}

void moveTet(){
    debug("moveTet()");
    while(1){
        //%cで取ると誤入力で複数とか改行だとかが面倒なのでこうする
        char temp[10];
        printf("input a key and push Enter:");
        scanf("%s", temp);
        Input key = temp[0];//最初のを採用

        debug_scast("input string is <%s>, key is the first one.", temp)
        //入力にしたがって動かす、動かせなければ操作は行われずnoが戻ってくる
        Result result = __updateCurrentTetState(key);

        if(result == ok){
            //sound(move_tet_ok);
            break;
        }else if(result == no){
            //sound(move_tet_no);
            continue;
        }
    }
}

//底についているか。現状は衝突していない前提
Result checkTetReachBottom(){
    debug("checkTetReachBottom()");
    //一番下まで落下させるやつとくっつけたくなるがこれは他の操作でも使うので
    
    Result is_bottom;

    //一段下げてみて衝突してたら今の場所が底についている。
    //下に行くには+1だああああああ。二個出る問題はここが原因
    Point under_point = {current_point.x, current_point.y + 1};
    Result under_is_free = __checkCollision(under_point, current_arg_id);
    debug_icast("current_arg_id = %d", current_arg_id);
    if(under_is_free == yes){
        is_bottom = no;
    }else if(under_is_free == no){
        is_bottom = yes;
    }
    return is_bottom;
}

//updateColorFieldとの違いは書き込み対象がgame_logic_fieldかどうかと他少し
void currentTetToFieldColor(){
    debug("currntTetToFieldColor()");
      //現在の回転の情報を受取る
    const int tet_height = arg_data_list[current_arg_id]->height;
    const int tet_width = arg_data_list[current_arg_id]->width;
    const int *order = arg_data_list[current_arg_id]->print_order;

    int this_x, this_y;
    int tet_arg_place_num = 0;
    for(int h = 0;h < tet_height;h++){
        this_y = h + current_point.y;
        for(int w = 0;w < tet_width;w++){
            this_x = w + current_point.x;

            //この場所に表示すべきTetの色を取得
            const int tet_original_place_num = order[tet_arg_place_num];
            const ColorID this_color_id = current_tet.body[tet_original_place_num];
            if(this_color_id != nocolor_id){
                if(game_logic_field[this_y][this_x] != nocolor_id){
                    printf("tried to write colorID = %d at point{h = %d,w = %d},\n", this_color_id, this_y,this_x);
                    printf("but at game_logic_field[h = %d][w = %d],\n", this_y, this_x);
                    printf("there are already colorID = %d exists.\n", color_field_to_show[this_y][this_x]);
                    fatailError("Collision Conflict at currentTetToFieldColor(). There may be a bug in checkCollision() or in other code.");
                }
                //[y][x]。Tetの色を合成
                game_logic_field[this_y][this_x] = this_color_id;
            }

            tet_arg_place_num++;
        }
    }
}

void __slideRow(int delete_row_id){
    debug("__slideRow");
    //該当行から上に向かって１行まで（0行を1行目にコピーするところまで）実行
    for(int h = delete_row_id;h > 0;h--){
        for(int w = 0;w < WIDTH;w++){
            //そのマスのすぐ上を取ってくる
            game_logic_field[h][w] = game_logic_field[h-1][w];
        }
    }
    //0行目は0埋め
    for(int w = 0;w < WIDTH;w++){
        game_logic_field[0][w] = 0;
    }
}

void removeCompletedRow(){
    debug("removeCompletedRow()");
    //まず初期値はyesでありwhileを通る
    Result there_may_be_bingo = yes;

    //複数行が一気に消える可能性もあるのでwhile
    while(there_may_be_bingo){
        //何も見つからなければこのnoがそのまま残りスキャンは終了
        there_may_be_bingo = no;

        //縦に一周
        for(int h = 0;h < HEIGHT;h++){
            //check if it had been bingo
            for(int w = 0;w < WIDTH;w++){
                //0が1つでも見つかればそれはビンゴでない。
                if(game_logic_field[h][w] == nocolor_id){
                    break;

                //最後までbreakせずに走ったらこれはビンゴ
                }else if(w == WIDTH-1){
                    //ビンゴが見つかればまだその下にある可能性があるのでyesに変える
                    there_may_be_bingo = yes;
                    
                    //この行＝hを削除して上のものを下げる
                    __slideRow(h);
                }
            }
        }
    }
}

//currentTetをlogicFieldに重ねてcolorFieldを作る
void updateColorField(){
    debug("updateColorField()");
    //衝突判定と似ているが向こうは全て終わるまでOKかわからないので同時に合成してはいけない
    //まず前回のを消す。
    for(int h = 0;h < HEIGHT;h++){
        for(int w = 0;w < WIDTH;w++){
            color_field_to_show[h][w] = game_logic_field[h][w];
        }
    }
    //現在の回転の情報を受け取る
    const int tet_height = arg_data_list[current_arg_id]->height;
    const int tet_width = arg_data_list[current_arg_id]->width;
    const int *order = arg_data_list[current_arg_id]->print_order;

    int this_x, this_y;
    int tet_arg_place_num = 0;
    for(int h = 0;h < tet_height;h++){
        this_y = h + current_point.y;
        for(int w = 0;w < tet_width;w++){
            this_x = w + current_point.x;

            //この場所に表示すべきTetの色を取得
            const int tet_original_place_num = order[tet_arg_place_num];
            const ColorID this_color_id = current_tet.body[tet_original_place_num];
            if(this_color_id != nocolor_id){
                if(color_field_to_show[this_y][this_x] != nocolor_id){
                    printf("tried to write colorID = %d at point{h = %d,w = %d},\n", this_color_id, this_y,this_x);
                    printf("but at game_logic_field[h = %d][w = %d],\n", this_y, this_x);
                    printf("there are already colorID = %d exists.\n", color_field_to_show[this_y][this_x]);
                    fatailError("Collision Conflict at updateColorField(). There may be a bug in checkCollision() or in other code.");
                }
                //[y][x]。Tetの色を合成
                color_field_to_show[this_y][this_x] = this_color_id;
            }

            tet_arg_place_num++;
        }
    }
}

//print bufferとdisplay stringsの両方を制御する。colorfieldまで出来てること前提。
void displayShow(){
    debug("displayShow()");
    //別々な生成は今回はパスで。遅かったらやる
    setAllDisplayStrings();
    //カラム別な二次元配列の文字列を1つの長い文字列へ
    makePrintBufferFromDisplayString();;
    //表示
    writePrintBuffer();
}

Result startCycle(){
    debug("--start-- startCycle()");
    debug("~~~show current tet");
    #ifdef is_debug
    debug_showTetromino(current_tet);
    #endif
    debug("~~~This is current tet");
//printf("next_tet_id[%d] is %d", 1, next_tet_id[1]);
    //Tetが底についてるか確認
    Result result = checkTetReachBottom();
    if(result == yes){
        debug("tet touched bottom !!!");
        //current tetの位置をFieldの配列へ転写
        currentTetToFieldColor();
        //完成した行を取り除いて、それより上を落下
        removeCompletedRow();
        //新規生成
        Result spawn_suceed = spawnNewTet();
        if(spawn_suceed == no){
            //ゲームオーバー
            debug("falied to spawn new tet. this should mean game over.");
            return no;
        }
        //得点加算
        score++;
    }
//printf("next_tet_id[%d] is %d", 1, next_tet_id[1]);
    //Tetと背景を合成する
    updateColorField();
//printf("3 next_tet_id[%d] is %d", 1, next_tet_id[1]);
    ///描画を更新
    displayShow();
//printf("4 next_tet_id[%d] is %d", 1, next_tet_id[1]); 
    //contiune_gameの関係上さいごにしないとｃを押しても１回動くことになる
    //入力を１つ受けて操作をする
    moveTet();
//printf("5 next_tet_id[%d] is %d", 1, next_tet_id[1]);
    //正常終了
    return ok;
}

/*
void loop_windows(){

}

void loop_linux(){

}*/
//ーーライフサイクルーーーーーーーーーーーーーーーーーーーー
void onCreate(){
    debug("***onCreate****");
    
    printf("Welcome to Console Tetoris!!\n");
    printf("%sWarning : this program flash your console text.%s\n", ColorStr.orange ,ColorStr.none);

    //os設定はもはや関係ない
    //int os_type = inputOsType();
    int os_type = 0;//win
    setOsType(os_type);


    //テトリスの盤面とそのStrings
    initializeField();

    //描画する文字列をつなげる作業場
    setPrintBuffer();

    //ランダム用
    //乱数の種を時刻でセット
    srand((unsigned int)time(NULL));

    //右側に表示する次のTetrominoリストをランダムに全て埋める
    initializeNextTetIDList();
    //debug_icast("next_tet_id[0] is %d", next_tet_id[0]);//正常
    //描画範囲が画面に収まってるか確認
}



void onGameStart(){
    debug("***onGameStart***");
    score = 0;

    //onCreateのは描画範囲の確認に使ったので再取得
    initializeNextTetIDList();

    //ゲーム開始時間を記録
    start_time = time(NULL);

    //continue_gameはｃキーが押されるとnoになる
    while(continue_game){
        
        //system("cls");
        //for(int i = 0;i < DEFAULT_ACTION_TO_DO;i++){
            /*//これしないと最大３回動く
            if(continue_game == no){
                break;
            }*/
            //この関数が主要処理を持ってる
            Result result = startCycle();
            //ここだあああ。=が一個になってた。コンパイラ指摘してくれ～～
            if(result == no){
                printf("%s<< GAME OVER >> field is full!%s\n", ColorStr.orange, ColorStr.none);
                return;//voidのreturn
            }
        //}

        time_t now_time = time(NULL);

        //時間の引き算逆にしてた（）
        //時間0のままなのここでint宣言つけてたからだ。
        esped_time = (int)(now_time - start_time);
        /*if(esped_time > 3){
            //return no means game over
            printf("%s<< GAME OVER >> oh! you do too slow!%s", ColorStr.mazenta, ColorStr.none);
            return;//voidのreturn
        }*/
    }

}



void onDestroy(){
    debug("***onDestroy***");

    int min = esped_time/60;
    int sec = esped_time%60;
    printf("%sNow, game finished.%s\n", ColorStr.green, ColorStr.none);
    printf("%sYout score is %05d and elapsed time is %02dmin, %02dsec!!%s\n", ColorStr.green, score , min, sec, ColorStr.none);

    freePrintBuffer();


    exit(EXIT_SUCCESS);
}


//ーーデバックーーーーーーーーーーーー

void debug_tetrominoPrinter_Arg(TetID tet_id, ArgID arg_id){
    debug("--start-- tetrominoPrinter_Arg()");
    printf("tetromino_id : %d, arg_id :%d\n", tet_id, arg_id);
    //tet_listはコンパイル時にサイズ決まってるためにポインタにしてある
    Tetromino tet = *tet_list[tet_id];

    //4*2か2*4の表示
    //アロー->なのは構造体がポインタとして配列に入っているかららしい。
    const int height = arg_data_list[arg_id]->height;
    const int width = arg_data_list[arg_id]->width;
    //ポインタで受ければforでコピーしなくて済む
    const int *order = arg_data_list[arg_id]->print_order;
    
    //例えばwidth=2,height=4ならorderから二個出力した後、改行される。これを四回繰り返す
    int place_id_num = 0;
    for(int h = 0;h < height;h++){
        for(int w = 0;w < width;w++){
            //配列をかませる
            const int point_id = order[place_id_num];

            //配列に記載された場所のマスに書かれた色を出す
            const ColorID color_id = tet.body[point_id];
            printBufferAdd(color_list[color_id]);
            
            
            char debug[3];
            sprintf(debug, "%d%d", place_id_num, point_id); 
            printBufferAdd(debug);

            //カウント
            place_id_num++;
        }
        printBufferAdd(ColorStr.none);
        printBufferAdd("\n");
    }
            
    
    writePrintBuffer();
}

void debug_showTetromino(Tetromino tet){
    debug("--start-- tetrominoPrinter_Arg()");
    ArgID arg_id = arg0_id;

    //4*2か2*4の表示
    //アロー->なのは構造体がポインタとして配列に入っているかららしい。
    const int height = arg_data_list[arg_id]->height;
    const int width = arg_data_list[arg_id]->width;
    //ポインタで受ければforでコピーしなくて済む
    const int *order = arg_data_list[arg_id]->print_order;
    
    //例えばwidth=2,height=4ならorderから二個出力した後、改行される。これを四回繰り返す
    int place_id_num = 0;
    for(int h = 0;h < height;h++){
        for(int w = 0;w < width;w++){
            //配列をかませる
            const int point_id = order[place_id_num];

            //配列に記載された場所のマスに書かれた色を出す
            const ColorID color_id = tet.body[point_id];
            printBufferAdd(color_list[color_id]);
            
            
            char debug[3];
            sprintf(debug, "%d%d", place_id_num, point_id); 
            printBufferAdd(debug);

            //カウント
            place_id_num++;
        }
        printBufferAdd(ColorStr.none);
        printBufferAdd("\n");
    }
            
    
    writePrintBuffer();
}

void debug_tetrominoPrinter(TetID tet_id){
    debug_tetrominoPrinter_Arg(tet_id, arg0_id);
}

void debug_printAll(){
    debug("--start-- printAll()");
    const int Tet_num = 7;
    for(TetID t_id = 0;t_id < 7;t_id++){
        debug_tetrominoPrinter(t_id);
    }
}

void debug_printAll_Arg(){
    debug("--start-- printAll_Arg()");
    const int Tet_num = 7;
    const int Arg_num = 4;
    for(TetID t_id = 0;t_id < Tet_num;t_id++){
        
        for(ArgID a_id = 0;a_id < Arg_num;a_id++){
            debug_tetrominoPrinter_Arg(t_id, a_id );
        }
        
    }
}



int main(){
    onCreate();
    //printf("next_tet_id[%d] is %d", 1, next_tet_id[1]);
    //debug_icast("next_tet_id[0] is %d", next_tet_id[0]);
    
    //debug_printAll();
    //debug_printAll_Arg();

    onGameStart();

    //setAllDisplayStrings();
    //makePrintBufferFromDisplayString();
    //writePrintBuffer();


    onDestroy();
    return 0;
}
