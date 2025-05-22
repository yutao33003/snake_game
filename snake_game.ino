#include <LedControl.h>
#include <TM1637Display.h>

#define LEDMATRIX 0
#define CLK 8  
#define DIO 9

// 設定TM1637腳位設定
TM1637Display digitDisplay(CLK, DIO);
// 設定MAX7219腳位設定(DIN,CLK,CS)
LedControl lc = LedControl(11, 13, 10, 1);

// 按鈕對應腳位
const int upBtn = 2;
const int downBtn = 3;
const int leftBtn = 4;
const int rightBtn = 5;

// 蜂鳴器與電位器
const int buzzerPin = 6;
const int potPin = A0;

// 地圖大小 
const byte WIDTH = 8;
const byte HEIGHT = 8;

// 貪吃蛇建構子內設定即時位置初始變數資料
struct Point{
  byte x,y;
};

Point snake[64]; 
int snakeLength = 3;
int dx = 1;
int dy = 0;

Point food;

// 初始化level
int level = 1;

bool gameStarted = false;
bool gameOver = false;
bool gameClear = false;

int blinkStep = 0;

// 用來記錄上一次蛇移動的時間點(ms),會返回arduino開機以來過了多久
unsigned long lastMoveTime = 0;

int currentLevel = 1;
int autoLevelupThreshold = 2;// 吃到兩個食物才會增加一個難度
int foodEatenSinceLastLevelUp = 0; // 當前吃了多少個食物
// 紀錄確認數字燈上難度的時間差
unsigned long lastLevelCheckTime = 0;
unsigned long lastBlinkTime = 0;

// 管理閃爍的變數
bool isFoodBlinking = false;
unsigned long blinkStartTime = 0;

// 蛇每次移動的間隔時間，預設500s
int moveInterval = 500;
// 設定最大長度
const int maxLength = 32;

// 對貪吃蛇遊戲初始化設定
void setup() {

  // 初始化難度
  level = readDifficulty(); 
  currentLevel = level;
  
  // 初始化 LED 矩陣與數字顯示器
  lc.shutdown(LEDMATRIX, false);
  lc.setIntensity(LEDMATRIX, 8);
  lc.clearDisplay(LEDMATRIX);

  // 初始化 TM1637 數字顯示器
  digitDisplay.setBrightness(7); // 設置亮度 (0-7)
  digitDisplay.clear();

  // 顯示難度在顯示器上
  showDifficultyLevel(level);
  
  // 將上下左右按鈕的設成輸入模式啟用內部上拉電阻。
  pinMode(upBtn, INPUT_PULLUP);
  pinMode(downBtn, INPUT_PULLUP);
  pinMode(leftBtn, INPUT_PULLUP);
  pinMode(rightBtn, INPUT_PULLUP);
  //輸出模式，之後可以用 `tone()` 來發聲。
  pinMode(buzzerPin, OUTPUT);
  
  snake[0] = {3, 4};
  snake[1] = {2, 4};
  snake[2] = {1, 4};
  
  // 呼叫開場畫面
  showStartScreen();

  generateFood();
  drawGame();
}

void loop() { // loop持續判斷
  if (gameStarted == false){
    updateDifficulty();
  }
  
  if (isFoodBlinking) {
    unsigned long now = millis();

    if (now - lastBlinkTime >= moveInterval) {
      switch (blinkStep) {
        case 0: lc.setLed(0, food.y, food.x, false); break;
        case 1: lc.setLed(0, food.y, food.x, true); break;
        case 2: lc.setLed(0, food.y, food.x, false); break;
        case 3: lc.setLed(0, food.y, food.x, true); break;
        case 4: lc.setLed(0, food.y, food.x, false); break;
        case 5:
          isFoodBlinking = false;
          blinkStep = 0;

          generateFood(); // 閃完再出現新食物
          playEatSound();
          return;
      }
      blinkStep++;
      lastBlinkTime = now;
    }
    return; // blink 中，不做其他事
  }
  
  if (gameOver) {
    showGameOver();
    return;
  }
  if (gameClear) {
    showGameClear();
    return;
  }

  checkInput();

  // 檢查當前程式碼與上次移動的時間差，判斷是否更新畫面
  if (millis() - lastMoveTime > moveInterval) {
    moveSnake();
    drawGame();
    lastMoveTime = millis();
  }
}

// 開場歡迎畫面
void showStartScreen(){
  // 實作開場畫面
  lc.clearDisplay(LEDMATRIX);
  // 可以加入一些簡單動畫
  delay(1000);
}

// 生成食物
void generateFood(){
  bool valid = false;

  // 尚未生成食物則進入迴圈
  while (!valid){
    food.x = random(0,WIDTH);
    food.y = random(0,HEIGHT);
    valid = true;
    //判斷食物生成的位置是否與snake相撞
    for( int i = 0; i < snakeLength; i++){
      if(snake[i].x == food.x && snake[i].y == food.y){
        valid = false;
        break;
      }
    }
  }
}

// 畫出貪吃蛇地圖
void drawGame(){
  lc.clearDisplay(LEDMATRIX);
  
  // 於MAX7219 上印出蛇身
  for(int i = 0; i < snakeLength; i++){
    lc.setLed(0, snake[i].y, snake[i].x,true);
  }

  // 顯示食物（除非正在閃爍中）
  if (!isFoodBlinking) {
    lc.setLed(LEDMATRIX, food.y, food.x, true);
  }
}

//使用可變電阻調整難度，並顯示於LED數字燈
void updateDifficulty(){
  // 每500ms檢查一次
  if (millis() - lastLevelCheckTime > 500){
    int potLevel = readDifficulty();
    if (abs(potLevel - currentLevel) >= 2){
      currentLevel = potLevel;
      showDifficultyLevel(currentLevel);
      // 速度調整
      // 蛇移動的間隔會越來愈少600->100
      moveInterval = map(currentLevel, 1, 9, 600, 100);
    }
    // 更新levelchecktime的時間
    lastLevelCheckTime = millis();
  }  
}

// 讀取可變電阻上轉動的難度區間，並呼叫showDifficultyLevel顯示在數字燈上
int readDifficulty(){
  int val = analogRead(potPin);// 0~1023
  int level = map(val, 0, 1023, 1, 9);// 對應到難度 1~9
  return constrain(level, 1, 9);
}

// 顯示在數字LED上的難度
void showDifficultyLevel(int level){
  // 創建一個數字顯示數組，只在最右側顯示難度數字
  uint8_t data[] = {0, 0, 0, 0};
  data[3] = digitDisplay.encodeDigit(level);
  
  // 將數據顯示在TM1637上
  digitDisplay.setSegments(data);
}

// 按鈕邏輯，改變蛇的動態位置
void checkInput(){
  if (digitalRead(upBtn) == LOW && dy != 1) {
    dx = 0; dy = -1;
  } else if (digitalRead(downBtn) == LOW && dy != -1) {
    dx = 0; dy = 1;
  } else if (digitalRead(leftBtn) == LOW && dx != 1) {
    dx = -1; dy = 0;
  } else if (digitalRead(rightBtn) == LOW && dx != -1) {
    dx = 1; dy = 0;
  }
}

// 判斷貪吃蛇動態位置與闖關的成敗
void moveSnake() {
  Point newHead = { (byte)(snake[0].x + dx), (byte)(snake[0].y + dy) };

  //碰到邊界死亡
  if (newHead.x >= WIDTH || newHead.y >= HEIGHT || newHead.x < 0 || newHead.y < 0) {
    gameOver = true;
    return;
  }

  //碰到自己死亡
  for (int i = 1; i < snakeLength; i++) {
    if (snake[i].x == newHead.x && snake[i].y == newHead.y) {
      gameOver = true;
      return;
    }
  }

  // 更新蛇的位置
  for (int i = snakeLength; i > 0; i--) {
    snake[i] = snake[i - 1];
  }
  snake[0] = newHead;

  // 成功吃到食物
  if (newHead.x == food.x && newHead.y == food.y) {
    eatFood();
  }
  
  // 檢查是否達到最大長度（勝利條件）
  if (snakeLength >= maxLength) {
    gameClear = true;
    return; // 可選：避免多餘處理
  }
}

// 吃到食物後提升難度
void eatFood() {
  snakeLength++;
  
  foodEatenSinceLastLevelUp++;

  if (!gameStarted) {
    gameStarted = true;
  }

  // 判斷達到閃爍條件
  if (foodEatenSinceLastLevelUp >= autoLevelupThreshold) {
    // 進入閃爍模式
    isFoodBlinking = true;
    blinkStep = 0;
    lastBlinkTime = millis();
    
    // 判斷難度是否滿足難度提升的條件
    if (currentLevel < 9) {
      currentLevel++;
      level = currentLevel;
      foodEatenSinceLastLevelUp = 0; // 重置計數器
      showDifficultyLevel(level);
      moveInterval = map(currentLevel, 1, 9, 600, 100);
    }
    
    foodEatenSinceLastLevelUp = 0; // 重置計數器
    return; // 不馬上產生新食物，由閃爍結束後生成
  }
  
// 正常吃食物，直接生成新的
  generateFood();
  playEatSound();
}


// 顯示闖關失敗
void showGameOver() {
  lc.clearDisplay(LEDMATRIX);
  playCrashSound();

   // 在 TM1637 上顯示 "OVER"（使用7段顯示器可顯示的文字）
  uint8_t data[] = {
    0x3F,           // O
    0x3E,  // V (類似於 U)
    0x79,  // E
    0x50   // r
  };
  digitDisplay.setSegments(data);
  
  for (int i = 0; i < 3; i++) {
    lc.setRow(0, i + 2, 0b11111111); // 閃幾排燈當作Game Over效果
    delay(300);
    lc.clearDisplay(LEDMATRIX);
    delay(300);
  }

  // 等待按鈕按下重新開始
  waitForRestart();
}

// 顯示遊戲結束
void showGameClear() {

  // 清除遊戲畫面
  lc.clearDisplay(LEDMATRIX);
  // 播放勝利的音樂
  playVictorySound();

  // 在 TM1637 上顯示 "WIN "
  uint8_t data[] = {
    SEG_F | SEG_E | SEG_D | SEG_C | SEG_B | SEG_G,  // W (類似於 ω)
    SEG_E | SEG_F,                                  // I
    SEG_E | SEG_G | SEG_C,                          // n
    0                                              // 空白
  };
  
  // 數字燈的閃爍效果
  for (int i = 0; i < 5; i++){
    digitDisplay.setSegments(data);
    delay(300);
    digitDisplay.clear();
    delay(300);
  }
  
  for (int i = 0; i < 5; i++) {
    // 顯示的每一行設置為全亮，再清除顯示並延遲300毫秒
    for (int r = 0; r < 8; r++) lc.setRow(0, r, 0b11111111);
    delay(300);
    lc.clearDisplay(LEDMATRIX);
    delay(300);
  }

  // 等待按鈕按下重新開始
  waitForRestart();
  
}

// 播放吃到食物的音樂
void playEatSound() {
  tone(buzzerPin, 1000, 100);
  delay(120);
  noTone(buzzerPin);
}

// 播放撞擊的音樂
void playCrashSound() {
  for (int i = 0; i < 3; i++) {
    tone(buzzerPin, 300, 150);
    delay(180);
    noTone(buzzerPin);
  }
}

// 播放闖關成功的音樂
void playVictorySound() {
  int melody[] = { 523, 659, 783, 1046 };
  int durations[] = { 200, 200, 200, 400 };
  for (int i = 0; i < 4; i++) {
    tone(buzzerPin, melody[i], durations[i]);
    delay(durations[i] + 50);
  }
  noTone(buzzerPin);
}

// 檢測按鈕輸入並重啟遊戲
void waitForRestart() {
  // 顯示簡單的提示（例如閃爍特定LED）
  bool blinkState = false;
  unsigned long lastBlinkTime = 0;
  gameStarted = false;
  
  // 持續檢測按鈕輸入
  while (true) {
    // 閃爍提示燈
    if (millis() - lastBlinkTime > 500) {
      blinkState = !blinkState;
      lc.clearDisplay(LEDMATRIX);
      
      // 顯示一個閃爍的方塊或圖案，提示玩家按按鈕重新開始
      if (blinkState) {
        for (int i = 3; i < 5; i++) {
          for (int j = 3; j < 5; j++) {
            lc.setLed(0, i, j, true);
          }
        }
      }
      
      lastBlinkTime = millis();
    }
    
    // 檢測任意按鈕是否被按下
    if (digitalRead(upBtn) == LOW || 
        digitalRead(downBtn) == LOW || 
        digitalRead(leftBtn) == LOW || 
        digitalRead(rightBtn) == LOW) {
      // 重新初始化遊戲
      resetGame();
      return; // 退出此函數，回到 loop()
    }
    
    delay(50); // 小延遲避免讀取過快
  }
}

//重置遊戲狀態
void resetGame() {

  // 重置蛇的位置和長度
  snake[0] = {3, 4};
  snake[1] = {2, 4};
  snake[2] = {1, 4};
  snakeLength = 3;
  
  // 重置移動方向
  dx = 1;
  dy = 0;
  
  // 重置遊戲狀態
  gameOver = false;
  gameClear = false;
  gameStarted = true;

  // 重置食物相關變數
  isFoodBlinking = false;
  foodEatenSinceLastLevelUp = 0;
  blinkStep = 0;
  
  // 更新難度
  level = currentLevel;
  showDifficultyLevel(level);
  moveInterval = map(level, 1, 9, 600, 100);
  
  // 清除並重繪遊戲畫面
  lc.clearDisplay(LEDMATRIX);
  digitDisplay.clear();
  
  // 顯示當前難度
  showDifficultyLevel(level);
  
  // 重新生成食物
  generateFood();
  drawGame();

  // 重置時間記錄
  lastMoveTime = millis();
  lastLevelCheckTime = millis();
  lastBlinkTime = millis();
}
