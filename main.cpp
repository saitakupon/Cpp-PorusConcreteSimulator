#include "cstdio" // 標準ライブラリ(print)
#include "cmath" // 数値計算ライブラリ(abs, pow, fmod)
#include "iostream"
#include "fstream"


//---------- パラメータ群 ----------//
int TOTAL_LENGTH = 420;
int NODE_NUM = 210;
int DX = TOTAL_LENGTH / NODE_NUM;
double K = 1.7; // 非線形透水係数
double M = 0.7; // 非線形係数
double SE = 0.25; // 空隙率
double S0 = 0; // 勾配
double END_BORDER = 0.01; // 収束条件
double DT = 0.001;
double TIME_OF_CALC = 10000.0;
double TIME_OF_RAIN = 4800.0;
double AMOUNT_OF_RAIN = 67.5 / 10.0 / 3600.0;
double HBC1 = 12.50; // 初期水位
double HBC2 = 22.157; // 止水版の高さ
//----------------------------------//


// 配列をコピーする
void arrayCopy (double a[], const double b[], int length) {
    for (int i = 0; i < length; i++) {
        a[i] = b[i];
    }
}

// 結果の出力
void showResult(double time, double height[]) {
    if (time == 0.0) {
        printf("%8s%8s%8s%8s%8s%8s%8s%8s\n", "Time", "0cm", "30cm", "90cm", "140cm", "210cm", "280cm", "350cm");
    }
    printf("%8.0lf%8.3lf%8.3lf%8.3lf%8.3lf%8.3lf%8.3lf%8.3lf\n", time, height[0], height[int(30 / DX)], height[int(90 / DX)], height[int(140 / DX)], height[int(210 / DX)], height[int(280 / DX)], height[int(350 / DX)]);
}

// 流量の計算
void calcFlux(double height[], double flux[]) {
    for (int i = NODE_NUM - 1; i >= 0; i--) {
        if (i == NODE_NUM - 1) {
            height[NODE_NUM] = height[NODE_NUM - 1] - (S0 * DX);
        }
        double heightMean = (height[i] + height[i + 1]) / 2.0;
        double heightGrade = std::abs((height[i + 1] - height[i]) / DX + S0);
        flux[i] = K * heightMean * std::pow(heightGrade, M);
        if (height[i] > height[i + 1]) {
            flux[i] = 0.0;
        }
    }
}

// 水位の計算
void calcHeight(const double height[], const double flux[], double rain, double afterHeight[]) {
    for (int i = NODE_NUM - 1; i > 0; i--) {
        afterHeight[i] = height[i] - ((DT / DX / SE) * (flux[i - 1] - flux[i])) + ((DT / SE) * rain);
    }
}

// 下流端の計算
void calcEdge(double afterHeight[], double time, double dropTime) {
    if (afterHeight[1] < HBC2) {
        afterHeight[0] = afterHeight[1] + S0 * DX;
    } else {
        double gradeBorder;
        if ((time - dropTime) > 16.0 && dropTime > 0) {
            gradeBorder = 0.0004 * std::log(time - dropTime) - 0.0010;
        } else {
            gradeBorder = 0.0010;
        }
        afterHeight[0] = afterHeight[int(210 / DX)] - (NODE_NUM * gradeBorder);
    }
}

// 収束の判定
bool judgeEnd(double height[], double beforeHeight[], double afterHeight[], double time) {
    bool retry = true;
    double diffMax = 0.0;
    for (int i = 1; i < NODE_NUM; i++) {
        double diff = std::abs(afterHeight[i] - beforeHeight[i]);
        if (diff > diffMax) {
            diffMax = diff;
        }
    }
    if (diffMax < END_BORDER) {
        arrayCopy(height, afterHeight, NODE_NUM + 1);
        if(std::fmod(time, 100) < DT) {
            showResult(time, height);
        }
        retry = false;
    } else {
        arrayCopy(beforeHeight, afterHeight, NODE_NUM + 1);
    }
    return retry;
}


// コマンドライン引数を使用したい場合は-> int main(int argc, char *argv[])
// メインの関数(デフォルトで実行される)
int main() {
    std::ofstream ofs("result.csv");
    ofs << "Time" << "," <<"0cm" << "," << "30cm" << "," << "90cm" << "," << "140cm" << "," << "210cm" << "," << "280cm" << "," << "350cm" << std::endl;
    double height[NODE_NUM + 1]; // height[0]->止水版上の水位
    double beforeHeight[NODE_NUM + 1];
    double afterHeight[NODE_NUM + 1];
    double flux[NODE_NUM + 1]; // flux[0]->止水版上の流量
    double beforeFlux[NODE_NUM + 1];
    for (int i = 0; i < NODE_NUM + 1; i++) {
        height[i] = HBC1;
        beforeHeight[i] = HBC1;
        afterHeight[i] = HBC1;
        flux[i] = 0;
        beforeFlux[i] = 0;
    }
    double time = 0.0;
    double dropTime = 0.0;
    bool dropFlag = false;
    double rain = AMOUNT_OF_RAIN;
    showResult(time, height);
    ofs << int(time) << "," << height[0] << "," << height[int(30 / DX)] << "," << height[int(90 / DX)] << "," << height[int(140 / DX)] << "," << height[int(210 / DX)] << "," << height[int(280 / DX)] << "," << height[int(350 / DX)] << std::endl;

    while (time < TIME_OF_CALC) {
        time += DT;
        if (!dropFlag && height[0] >= HBC2) {
            dropTime = time;
            dropFlag = true;
        }
        if (time > TIME_OF_RAIN) {
            rain = 0.0;
        }
        arrayCopy(beforeHeight, height, NODE_NUM + 1); // 元の水位をbeforeHeightに保存
        arrayCopy(beforeFlux, flux, NODE_NUM + 1); // 元の流量をbeforeFluxに保存
        bool retry = true;
        while (retry) {
            calcFlux(height, flux);
            calcHeight(height, flux, rain, afterHeight);
            calcEdge(afterHeight, time, dropTime);
            retry = judgeEnd(height, beforeHeight, afterHeight, time);
            if(!retry && std::fmod(time, 100.0) < DT) {
                ofs << int(time) << "," << height[0] << "," << height[int(30 / DX)] << "," << height[int(90 / DX)] << "," << height[int(140 / DX)] << "," << height[int(210 / DX)] << "," << height[int(280 / DX)] << "," << height[int(350 / DX)] << std::endl;
            }
        }
    }
    printf("end\n");
    return 0;
}
