## snake_game
This project is about the snake game. Players can use buttons to control the movement of the snake, eat food to make the snake longer, and the game ends when it hits a wall or itself.
## 專案使用材料
![image](https://github.com/user-attachments/assets/0fbd7216-7219-4755-bfde-c83c447df9f9)
## 貪吃蛇遊戲功能
1. 開始時有開場點陣動畫才進入遊戲。  
2. 玩家可以使用按鈕控制貪吃蛇方向(黃：左,紅：上, 藍：下, 白：右)  
3. 初始長度只有3，隨著難度上升**蛇身會變長**，並且速度**變快**(貪吃蛇移動的速度最初為**600ms**，隨著難度上升最快達**100ms**，隨著難度等比縮短)。  
4. 每升一個等級，需破兩關，第一關是食物定點，第二關則是**食物會隨著閃爍倒數**消失，出現在新位置，直到吃到才破關。  
5. **難度**可藉由可變電阻調整，顯示在**TM1637數字燈**上(難度1~9)(遊戲也可以調整)。  
6. 當闖關成功會有成功的音效與對應的點陣動畫，TM1637也會動態升等。  
7. 失敗時則會使用蜂鳴器發出撞擊聲音，提醒玩家在本關輸掉，TM1637顯示over，當按下上下左右任意按鈕則**重置遊戲**。  
8. 闖關成功則會顯示**WIN於TM1637**上，並調整等級，而蜂鳴器會發出**勝利的聲音**  
<br>
![S__28172290](https://github.com/user-attachments/assets/b9f86e2e-e098-4eaa-ad8d-bb5389f6fce8)
