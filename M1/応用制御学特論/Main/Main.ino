//参考記事
//P制御(https://monozukuri-c.com/mbase-pcontrol/)


#include <SoftwareSerial.h>


//PID制御(https://kurobekoblog.com/pid)

#define PID_taget 0.0 //PIDの目標値

#define target 100.0 //壁との距離 100cm

#define Kp 2.5 //比例ゲイン
#define Ki 0.0 //積分ゲイン
#define Kd 0.0 //微分ゲイン


//DCモータ
//右
#define PIN_RIGHT_IN1 7
#define PIN_RIGHT_IN2 8
#define PIN_RIGHT_VREF 9

//左
#define PIN_LEFT_IN1 4
#define PIN_LEFT_IN2 2
#define PIN_LEFT_VREF 10

//DCモータのスピード
int left_Speed;
int right_Speed;

//最大と最小のスピードを指定
int High_Speed = 255;
int Low_Speed = 220;


//サーボモータ
int penguin=3;
String servo_direction; //サーボモータの向き


//LiDAR
SoftwareSerial Serial1(12,11);

float dist; //actual distance measurements of LiDAR
int strength; //signal strength of LiDAR
float temprature; 
int check; //save check value
int i;
int uart[9]; //save data measured by LiDAR
const int HEADER=0x59; //frame header of data package


//ローパスフィルタ
float LPF;
float lastLPF;
float k = 0.1;



int flont_distance = 600; //正面との距離
int side_distance = 200; //左右との距離



//PID制御
int duty = 0;
float dt, preTime;
float x;
float P, I, D, preP;





//前進
void foward(int left_speed, int right_speed)
{
  digitalWrite(PIN_RIGHT_IN1,HIGH);
  digitalWrite(PIN_RIGHT_IN2,LOW);
  digitalWrite(PIN_LEFT_IN1,HIGH);
  digitalWrite(PIN_LEFT_IN2,LOW);
  analogWrite(PIN_LEFT_VREF,left_speed);
  analogWrite(PIN_RIGHT_VREF,right_speed); 
}


//右
void right(int left_speed, int right_speed)
{
  digitalWrite(PIN_RIGHT_IN1,LOW);
  digitalWrite(PIN_RIGHT_IN2,HIGH);
  digitalWrite(PIN_LEFT_IN1,HIGH);
  digitalWrite(PIN_LEFT_IN2,LOW);
  analogWrite(PIN_LEFT_VREF,left_speed);
  analogWrite(PIN_RIGHT_VREF,right_speed); 
}

//左
void left(int left_speed, int right_speed)
{
  digitalWrite(PIN_RIGHT_IN1,HIGH);
  digitalWrite(PIN_RIGHT_IN2,LOW);
  digitalWrite(PIN_LEFT_IN1,LOW);
  digitalWrite(PIN_LEFT_IN2,HIGH);
  analogWrite(PIN_LEFT_VREF,left_speed);
  analogWrite(PIN_RIGHT_VREF,right_speed); 
}

//後ろ
void back(int left_speed, int right_speed)
{
  digitalWrite(PIN_RIGHT_IN1,LOW);
  digitalWrite(PIN_RIGHT_IN2,HIGH);
  digitalWrite(PIN_LEFT_IN1,LOW);
  digitalWrite(PIN_LEFT_IN2,HIGH);
  analogWrite(PIN_LEFT_VREF,left_speed);
  analogWrite(PIN_RIGHT_VREF,right_speed); 
}


//停止
int stop_(int left_speed, int right_speed)
{
  digitalWrite(PIN_RIGHT_IN1,LOW);
  digitalWrite(PIN_RIGHT_IN2,LOW);
  digitalWrite(PIN_LEFT_IN1,LOW);
  digitalWrite(PIN_LEFT_IN2,LOW);
  analogWrite(PIN_LEFT_VREF,left_speed);
  analogWrite(PIN_RIGHT_VREF,right_speed); 
}



//サーボモータ
void penDash(int x)
{//xの値は0~180。
  int kyori = (x*10.25)+450;//角度からパルス幅への変換式
  digitalWrite(penguin,HIGH);
  delayMicroseconds(kyori);
  digitalWrite(penguin,LOW);
  delay(5);//速度　5~30くらいが良好。
}



//LiDAR
void LiDAR()
{
   if(Serial1.available())
  {
    if(Serial1.read()==HEADER)
    {
      uart[0]=HEADER;

      if(Serial1.read()==HEADER)
      {
        uart[1]=HEADER;

        for(i=2;i<9;i++)
        {
          uart[i]=Serial1.read();
        }

        check=uart[0]+uart[1]+uart[2]+uart[3]+uart[4]+uart[5]+uart[6]+uart[7];

        if(uart[8]==(check&0xff))
        {
          dist = float(uart[2]+uart[3]*256);
          
          strength=uart[4]+uart[5]*256;
          
          temprature=uart[6]+uart[7]*256;
          temprature=temprature/8-256;
        }
      }
    }
  }

  if(dist>800) dist=800;
  if(dist<0) dist=0;
}



void raw_pass_filter() //https://garchiving.com/lpf-by-program/
{
  LPF = (1 - k) * lastLPF + k * (float)dist;
  lastLPF = LPF;
}



void PID(float Distance)
{
  x = Distance;
  
  dt = (micros() - preTime) / 1000000.0;
  preTime = micros();
  P  = target - x;
  I += P * dt;
  D  = (P - preP) / dt;
  preP = P;

  duty += Kp * P + Ki * I + Kd * D;

  //if(duty<-10) duty = -10;
  //if(duty>10) duty = 10;
  
}




void setup() {
  // put your setup code here, to run once:

  //サーボモータ
  pinMode(penguin,OUTPUT);
  //タイヤ
  pinMode(PIN_RIGHT_IN1,OUTPUT); 
  pinMode(PIN_RIGHT_IN2,OUTPUT); 
  pinMode(PIN_LEFT_IN1,OUTPUT); 
  pinMode(PIN_LEFT_IN2,OUTPUT); 
  
  //LiDAR
  Serial.begin(9600);
  Serial1.begin(115200);


  penDash(20); //0°
  servo_direction = "right";
  delay(5000);
}




void loop() {

  
  LiDAR();
  raw_pass_filter();
  PID(LPF);
  
  if(servo_direction == "right") //もし，サーボが右を向いていたら，
  {


    foward(int((float(High_Speed-Low_Speed)/200.0)*LPF+Low_Speed), int((float(Low_Speed-High_Speed)/200.0)*LPF+High_Speed)); //正面に進む


    // if(LPF<100)
    // {
    //   penDash(55); //45°傾ける
    // }else
    // {
    //   penDash(20); //0°傾ける
    // }

    

    //PID制御    
    // if(duty == PID_taget)
    // {
    //   foward(222, 222); //正面に進む
    // }
    // else if(duty < PID_taget) //もし，targetよりも実測値が長かったら
    // {
    //   foward(240, 190); //右に傾ける
    //   //foward(int(0.325*LPF+190), int(-2.225*LPF+445)); //右に傾ける
    // }
    // else
    // {
    //   foward(190,240); //左に傾ける
    //   //foward(int(0.325*LPF+190), int(-0.325*LPF+255)); //左に傾ける
    // }



    // if(LPF == target)
    // {
    //   foward(230, 230); //正面に進む
    // }
    // else if(LPF > target) //30cm以上なら
    // {
    //   //foward(250, 220); //右に傾ける
    //   foward(250, 200); //右に傾ける
    // }
    // else
    // {
    //   //foward(200, 250); //左に傾ける
    //   foward(200, 250); //左に傾ける
    // }


    //階段がある箇所での処理
    // if(LPF > side_distance) //もし，2.0mよりも距離が大きくなったら．
    // {
    //   stop_(0,0);
      
    //   penDash(95); //正面を向く
    //   delay(2000);
    //   LiDAR();

    //   if(LPF > flont_distance) //もし，正面に障害物が無いなら
    //   {
    //     penDash(190); //左を向く
    //     servo_direction = "left";
    //     delay(2000);
    //   }

    // }


  }





  // if(servo_direction == "left") //もし，サーボが左を向いていたら，
  // {
  //   if(LPF > target) //30cm以上なら
  //   {
  //     foward(230, 255); //左に傾ける
  //   }
  //   else
  //   {
  //     foward(255, 230); //右に傾ける
  //   }

  //   //階段がある箇所での処理
  //   if(LPF > side_distance) //もし，2.0mよりも距離が大きくなったら．
  //   {
  //     stop_(0,0);
      
  //     penDash(95); //正面を向く
  //     delay(2000);
  //     LiDAR();
  //     if(LPF > flont_distance) //もし，正面に障害物が無いなら
  //     {
  //       penDash(20); //右を向く
  //       servo_direction = "right";
  //       delay(2000);
  //     }
  //   }
  // }



  //Serial.print("target = ");
  //Serial.print(target);
  //Serial.print("\t");
  //Serial.print("PID_taget = ");
  //Serial.print(PID_taget);
  //Serial.print("\t");
  //Serial.print("dist = ");
  //Serial.print(dist);
  //Serial.print("\t");
  //Serial.print("LPF = ");
  //Serial.print(LPF);
  //Serial.print("\t");
  //Serial.print("duty = ");
  //Serial.print(duty);
  //Serial.print("\n");



  // penDash(20); //0°
  // delay(2000);
  // LiDAR();
  
  // penDash(95); //90°
  // delay(2000);
  // LiDAR();
  
  // penDash(190); //180°
  // delay(2000);
  // LiDAR();
  
  // penDash(95); //90°
  // delay(2000);
  // LiDAR();
  
  // penDash(20); //0°
  // delay(2000);
  // LiDAR();

  // stop();

  // while(1)
  // {
    
  // }

  
}
