package org.example.ndk;

import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.annotation.SuppressLint;
import android.app.AlertDialog;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Deque;
import java.util.List;

public class PaintView extends View {
	class Shapes{
		int type;
		int color;
		float firstX;
		float firstY;
		float secondX;
		float secondY;
	}

    //프로젝트에서 사용하는 변수
    private Paint mPaint;
    private Path mPath;
    private float mX, mY;
    private Button colorPicker, unDo, reDo, toggleMode;
    private TextView colorDisplay, test, input;
    private Bitmap mBitmap;
    private Canvas mCanvas;
    private float startX, startY, endX, endY, prevX = -99999999, prevY = -99999999;
    private int Mode; //0 : Rectangle    1 : Line
    private boolean isDrawingLine = false; 
    private int n, curColor, curStrokeSize = 1;
    private String curColorName;
    private int switchValues[] = new int[9];
    
	Deque<Shapes> DrawingEvents = new ArrayDeque<Shapes>();
	Deque<Shapes> ReDos = new ArrayDeque<Shapes>();
	Deque<Shapes> Line = new ArrayDeque<Shapes>();
	
    private Deque<Bitmap> undoStack;
    private Deque<Bitmap> redoStack;
	
    private int idx;
    public PaintView(Context context) {
        super(context);
        initPaintView();
    }

    //초기화 진행
    private void initPaintView() {
        mPaint = new Paint();
        mPaint.setColor(Color.BLACK); 
        curColor = Color.BLACK;
        mPaint.setStyle(Paint.Style.FILL); 
        n = 0;
        Mode = 1;
        mPaint.setAntiAlias(true); // Smooth drawing
        mPath = new Path();
        curColorName = "BLACK";
        mPaint.setStrokeWidth(1);
        curStrokeSize = 1;
        undoStack = new ArrayDeque<Bitmap>();
        redoStack = new ArrayDeque<Bitmap>();
    }

    private OnNumberChangeListener numberChangeListener, modeChangeListener, strokeChangeListener;
    private OnCoordinatesChangeListener coordinatesChangeListener;

    // 리스너 인터페이스 정의
    public interface OnNumberChangeListener {
        void onNumberChanged(int newNumber);
    }
    public interface OnCoordinatesChangeListener {
        void onCoordinatesChanged(float startX, float startY, float endX, float endY);
    }
    
    //리스너와 인터페이스 연결
    public void setOnNumberChangeListener(OnNumberChangeListener listener) {
        this.numberChangeListener = listener;
    }
    
    public void setOnModeChangeListener(OnNumberChangeListener listener) {
        this.modeChangeListener = listener;
    }
    
    public void setOnStrokeChangeListener(OnNumberChangeListener listener) {
        this.strokeChangeListener = listener;
    }
    
    public void setOnCoordinatesChangeListener(OnCoordinatesChangeListener listener) {
        this.coordinatesChangeListener = listener;
    }
    
    //CSE4116_Final에서 선언한 버튼과 paintView를 연결할 수 있도록 한다. 
    public void SetButtons(Button __colorPicker, Button __unDo, Button __reDo, Button __toggleMode) {
        colorPicker = __colorPicker;
        reDo = __reDo;
        unDo = __unDo;
        toggleMode = __toggleMode;
    }

     //CSE4116_Final에서 선언한 textView와 paintView를 연결할 수 있도록 한다. 
    public void SetTextView(TextView __colorDisplay, TextView __input) {
        colorDisplay = __colorDisplay;
        input = __input;
    }

     public void SetTest(TextView __test, int __dev) {
        if(__dev < 0)
            __test.setText("failed\n");
        else
            __test.setText("success\n");
    }

    //CSE4116_Final.java에서 특정 입력이 push switch를 통해 주어진 경우, application에서의 갱신이 일어날 수 있도록 한다.
     public void undoFromMain(){
    	 if (undoStack.size() > 1) {
             redoStack.push(Bitmap.createBitmap(mBitmap)); // Save current state for redo
             undoStack.pop();
             Bitmap previousBitmap = undoStack.getFirst(); 
             mBitmap = previousBitmap.copy(previousBitmap.getConfig(), true);
             mCanvas = new Canvas(mBitmap);
          n = undoStack.size() - 1;
          input.setText("" + n);
             invalidate(); // Redraw the view
         }
	    // n이 변경될 때마다 리스너 호출
	    if (numberChangeListener != null) {
	    	numberChangeListener.onNumberChanged(n);
	    }
     }

     //CSE4116_Final.java에서 특정 입력이 push switch를 통해 주어진 경우, application에서의 갱신이 일어날 수 있도록 한다.
     public void redoFromMain(){
    	 if (!redoStack.isEmpty()) {
             Bitmap nextBitmap = redoStack.pop();
             undoStack.push(Bitmap.createBitmap(nextBitmap)); // Save current state for undo
             mBitmap = nextBitmap.copy(nextBitmap.getConfig(), true);
             mCanvas = new Canvas(mBitmap); 
             int curCount = undoStack.size() - 1;
             n = curCount;
             input.setText("" + curCount);
   		 	invalidate(); // Redraw the view
         }   
            
//            for (Shapes shape : DrawingEvents) {
//           	 mPaint.setColor(shape.color);
//                drawRectangle(shape.firstX, shape.firstY, shape.secondX, shape.secondY);
//                invalidate();
//            }

            // n이 변경될 때마다 리스너 호출
            if (numberChangeListener != null) {
                numberChangeListener.onNumberChanged(n);
            }
     }

     //CSE4116_Final.java에서 특정 입력이 push switch를 통해 주어진 경우, application에서의 갱신이 일어날 수 있도록 한다.
     //stroke size 변경과 관련된 push switch입력이 주어진 경우 
     public void incStrokeSize(){
    	if(Mode == 1 || Mode == 3){
    		 if(curStrokeSize < 9){
        		 curStrokeSize += 1;
        	 }
        	 if(Mode == 1)
        		 mPaint.setStrokeWidth(curStrokeSize * 2);
        	 else
        		 mPaint.setStrokeWidth(curStrokeSize * 4);
             if (strokeChangeListener != null) {
            	 strokeChangeListener.onNumberChanged(curStrokeSize);
             }
    	}
     }

     //CSE4116_Final.java에서 특정 입력이 push switch를 통해 주어진 경우, application에서의 갱신이 일어날 수 있도록 한다.
     //stroke size 변경과 관련된 push switch입력이 주어진 경우 
     public void decStrokeSize(){
    	 if(Mode == 1|| Mode == 3){
    		 if(curStrokeSize > 1){
        		 curStrokeSize -= 1;
        	 }
    		 
    		 if(Mode == 1)
        		 mPaint.setStrokeWidth(curStrokeSize * 2);
        	 else
        		 mPaint.setStrokeWidth(curStrokeSize * 4);
    		 
        	  if (strokeChangeListener != null) {
             	 strokeChangeListener.onNumberChanged(curStrokeSize);
              }
    	 }
     }
     
     //CSE4116_Final.java에서 특정 입력이 push switch를 통해 주어진 경우, application에서의 갱신이 일어날 수 있도록 한다.
     //reset과 관련된 push switch 입력이 주어진 경우 
     public void resetFromMain(){
    	initPaintView();
    	if (strokeChangeListener != null) {
        	 strokeChangeListener.onNumberChanged(curStrokeSize);
         }
    	 if (numberChangeListener != null) {
        	 numberChangeListener.onNumberChanged(n);
    	    }
    	 if (modeChangeListener != null) {
   	    	modeChangeListener.onNumberChanged(Mode);
   	    }
    	 
    	 while(!redoStack.isEmpty())
    		 redoStack.pop();
    	 while(!undoStack.isEmpty())
    		 undoStack.pop();
    	 mBitmap = Bitmap.createBitmap(getWidth(), getHeight(), Bitmap.Config.ARGB_8888);
         mCanvas = new Canvas(mBitmap);
         mCanvas.drawColor(Color.WHITE); // 화면을 흰색으로 초기화합니다.
         input.setText("" + n);
         updateColorDisplay(Color.BLACK, "Black");
         invalidate();
     }
     
     //application에서 사용하는 undo 함수. 가능한 경우 이전의 그림판 상태로 되돌린다.
     private void undo() {
    	    if (undoStack.size() > 1) {
	               redoStack.push(Bitmap.createBitmap(mBitmap)); // Save current state for redo
	               undoStack.pop();
	               Bitmap previousBitmap = undoStack.getFirst(); 
	               mBitmap = previousBitmap.copy(previousBitmap.getConfig(), true);
	               mCanvas = new Canvas(mBitmap);
                n = undoStack.size() - 1;
                input.setText("" + n);
	               invalidate(); // Redraw the view
	           }
    	    // n이 변경될 때마다 리스너 호출
    	    if (numberChangeListener != null) {
        	 numberChangeListener.onNumberChanged(n);
    	    }
    	}

        //application에서 사용하는 redo 함수. 가능한 경우 이전의 그림판 상태로 되돌린다.
    	private void redo() {
       	 if (!redoStack.isEmpty()) {
             Bitmap nextBitmap = redoStack.pop();
             undoStack.push(Bitmap.createBitmap(nextBitmap)); // Save current state for undo
             mBitmap = nextBitmap.copy(nextBitmap.getConfig(), true);
             mCanvas = new Canvas(mBitmap); 
             int curCount = undoStack.size() - 1;
             n = curCount;
             input.setText("" + curCount);
   		 	invalidate(); // Redraw the view
         }   
       	if (coordinatesChangeListener != null) {
            coordinatesChangeListener.onCoordinatesChanged(startX, startY, endX, endY);
        }
//            for (Shapes shape : DrawingEvents) {
//           	 mPaint.setColor(shape.color);
//                drawRectangle(shape.firstX, shape.firstY, shape.secondX, shape.secondY);
//                invalidate();
//            }

            // n이 변경될 때마다 리스너 호출
            if (numberChangeListener != null) {
                numberChangeListener.onNumberChanged(n);
            }
    	}

        //사용 X 
        public void setSwitchValues(int[] __values){
       	 for(int i = 0; i < 9; i++){
       		 switchValues[i] = __values[i];
       	 }
       	Log.d("switchValues", "switchValues Values: " + Arrays.toString(switchValues));
       	  // Check the values and perform undo/redo
       	    if (switchValues[0] == 1) {
       	        
       	    } else if (switchValues[1] == 1) {
       	    	
       	    }
        }
    
    //사용 X
    public void getCoordinates(float x1, float y1, float x2, float y2){
    	x1 = startX;
    	y1 = startY;
    	x2 = endX;
    	y2 = endY;
    }
    
    //paintView와 연결된 button이 클릭되었을 때의 event를 listener를 통해 정의 
    public void SetButtonListeners() {
        //색상 변경 
        colorPicker.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                //클릭 시 dialog를 생성 
                showColorPickerDialog();
            }
        });
        //undo
        unDo.setOnClickListener(new OnClickListener(){
        	 @Override
             public void onClick(View v) {
                //undo 실행
        		  undo();
             }
        });
        
        //redo
        reDo.setOnClickListener(new OnClickListener(){
       	 @Override
            public void onClick(View v) {
                //redo 실행
       		 	redo();
            }
       });
        
        //그림판 도구 변경
        toggleMode.setOnClickListener(new OnClickListener(){
          	 @Override
               public void onClick(View v) {
          		 Mode = (Mode + 1) % 4;
          		 if(Mode == 0){
          			 toggleMode.setText("Rectangle");
          			curStrokeSize = 1;
          			mPaint.setStrokeWidth(curStrokeSize);
          		 }
          		 else if(Mode == 1)
          			toggleMode.setText("Pen");
          		 else if(Mode == 2){
          			 toggleMode.setText("Oval");
          			curStrokeSize = 1;
          			mPaint.setStrokeWidth(curStrokeSize);
          		 }
          		 else{
          			 toggleMode.setText("Eraser");
          		 }
          		 
          		 Log.d("%s\n", curColorName);
          	    // n이 변경될 때마다 리스너 호출
             	  if (strokeChangeListener != null) {
                   	 strokeChangeListener.onNumberChanged(curStrokeSize);
                    }
          	    if (modeChangeListener != null) {
          	    	modeChangeListener.onNumberChanged(Mode);
          	    }
          	 }
          	
        });
    }
    
    //color picker button이 눌러졌을 때 dialog를 생성 
    private void showColorPickerDialog() {
        final int[] colors = new int[]{
                Color.RED, Color.GREEN, Color.BLUE, Color.YELLOW,
                Color.CYAN, Color.MAGENTA, Color.BLACK, Color.WHITE
        };

        final String[] colorNames = new String[]{
                "Red", "Green", "Blue", "Yellow",
                "Cyan", "Magenta", "Black", "White"
        };

        AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
        builder.setTitle("Pick a color");
        builder.setItems(colorNames, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                mPaint.setColor(colors[which]);
                updateColorDisplay(colors[which], colorNames[which]);
                curColor = colors[which];
                curColorName = colorNames[which];
            }
        });
        builder.show();
    }

    //현재 선택 중인 색상을 application의 화면 상의 textview에 출력할 수 있도록 한다. 
    private void updateColorDisplay(int color, String colorName) {
        if (colorDisplay != null) {
            String colorHex = String.format("#%06X", (0xFFFFFF & color));
            colorDisplay.setText("Current Color: " + colorName + " (" + colorHex + ")");
        }
    }
    
    void setActionNum(int __n) {
        __n = n;
    }
    
    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);
        mBitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
        mCanvas = new Canvas(mBitmap);
        mCanvas.drawColor(Color.WHITE);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        canvas.drawBitmap(mBitmap, 0, 0, null);
        canvas.drawPath(mPath, mPaint);
    }

    //android application에서 touch event가 발생했을 때의 처리를 담당하는 event listener 정의
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                //touch가 시작되는 경우

            	if(undoStack.isEmpty()){
                    Bitmap bitmapCopy = Bitmap.createBitmap(mBitmap.getWidth(), mBitmap.getHeight(), mBitmap.getConfig());
                    Canvas canvas = new Canvas(bitmapCopy);
                    canvas.drawBitmap(mBitmap, 0, 0, null);

                    // Push the copied bitmap to the undo stack
                    undoStack.push(bitmapCopy);
            	}
                startX = event.getX();
                startY = event.getY();
                
                //지우개라면 색상을 흰색으로, 아닌 경우 현재 선택 중인 색상으로 변경
                if(Mode == 3){
                	mPaint.setColor(Color.WHITE);
                }
                else{
                	mPaint.setColor(curColor);
                }
                
                if (Mode == 1 || Mode == 3) {
                    isDrawingLine = true;
                    mPaint.setStyle(Paint.Style.STROKE);
                    if (prevX == -99999999 && prevY == -99999999) {
                        mPath.reset(); // Reset path for new touch
                        mPath.moveTo(startX, startY); // Move to the starting point
                        mX = startX;
                        mY = startY;
                    }
                }
                invalidate();
                break;

            case MotionEvent.ACTION_MOVE:
                //손가락이 이동되는 동안

                //좌표 갱신
                endX = event.getX();
                endY = event.getY();
                if(endX >1000)
                	endX = 999;
                if(endY > 1000)
                	endY = 999;
                if (Mode == 1 || Mode == 3) {
                    isDrawingLine = true;
                    float dx = Math.abs(endX - mX);
                    float dy = Math.abs(endY - mY);
                    if (dx >= 4 || dy >= 4) {
                        mPath.lineTo(mX, mY); // Smooth curve
                        mX = endX;
                        mY = endY;
                    }
                } else if(Mode == 0) {
                    drawRectangle();
                }
                else
                	drawOval();
                
                //invalidate를 통해 변경 사항을 application 화면에 계속 반영
                invalidate();
                break;

            case MotionEvent.ACTION_UP:
                //touch 종료 시 

                //좌표 갱신
                endX = event.getX();
                endY = event.getY();
                if (Mode == 0) {
                    drawRectangle();
                } else if (Mode == 1 || Mode == 3) {
                    isDrawingLine = true;
                    mPath.lineTo(mX, mY); // Connect the path to the last point
                    mCanvas.drawPath(mPath, mPaint); // Draw current path onto the canvas (bitmap)
                    mPath.reset(); // Reset path
                    mPaint.setStyle(Paint.Style.FILL);
                }
                else
                	drawOval();
                
                //변경 사항을 application에 반영하고
                invalidate();

                // 비트맵 복사 및 저장. 현재 그림 파일의 정보를 deque 자료구조를 통해 관리할 수 있도록 한다. 
                saveBitmap();
                Log.d("drawing", "DrawEvent Values: " + startX + " " + startY + " " + endX + " " + endY);
                Log.d("curColorName", curColorName);
                // n이 변경될 때마다 리스너 호출
                if (numberChangeListener != null) {
                    numberChangeListener.onNumberChanged(n);
                }
                if (coordinatesChangeListener != null) {
                	coordinatesChangeListener.onCoordinatesChanged(startX, startY, endX, endY);
                }
                break;
        }
        return true;
    }

    //현재 application 상의 그림 정보를 bitmap으로 변환하여 관리하도록 하는 함수 
    private void saveBitmap() {
        if (mBitmap != null) {
            // Create a new bitmap from the current state of mBitmap
            Bitmap bitmapCopy = Bitmap.createBitmap(mBitmap.getWidth(), mBitmap.getHeight(), mBitmap.getConfig());
            Canvas canvas = new Canvas(bitmapCopy);
            canvas.drawBitmap(mBitmap, 0, 0, null);
           
            // Push the copied bitmap to the undo stack
            undoStack.push(bitmapCopy);
            n = undoStack.size() - 1;
            input.setText("" + n);
            // Clear the redo stack
            while (!redoStack.isEmpty()) {
                redoStack.pop();
            }
        } else {
            Log.e("Error", "mBitmap is null");
        }
    }
    
    private void drawLine(float startX, float startY, float endX, float endY){
    	mCanvas.drawLine(startX, startY, endX, endY, mPaint);
    }
    private void drawRectangle() {
        if (mCanvas != null) {
            mCanvas.drawRect(startX, startY, endX, endY, mPaint);
            while(ReDos.size() != 0)
            	ReDos.pop();
        }
    }
    private void drawOval(){
    	if (mCanvas != null) {
    		RectF rectf = new RectF(startX, startY, endX, endY);
            mCanvas.drawOval(rectf, mPaint);
            while(ReDos.size() != 0)
            	ReDos.pop();
        }
    }
    public void onCoordinatesChanged(float startX, float startY, float endX, float endY) {
        Log.d("CoordinateChange", "StartX: " + startX + " StartY: " + startY + " EndX: " + endX + " EndY: " + endY);
    }
    
    private void drawRectangle(float a, float b, float c, float d) {
        if (mCanvas != null) {
            mCanvas.drawRect(a, b, c, d, mPaint);
        }
    }
}