package org.example.ndk;

import android.os.Bundle;
import android.app.Activity;
import android.content.res.Resources;
import android.util.Log;
import android.view.Menu;
import android.view.Window;
import android.widget.Button;
import android.widget.TextView;
import android.widget.LinearLayout;
import android.content.Intent;
import android.view.View;
import android.view.View.OnClickListener;

import java.util.*;
import android.os.Handler;
import android.os.Looper;

public class CSE4116_Final extends Activity {
	static {
		System.loadLibrary("ndk-exam");
	}
	
	private Handler handler;
	
	int undoFlag = 0, redoFlag = 0, incStrokeFlag = 0, decStrokeFlag = 0, resetFlag = 1;
	
	String[] modeArr = {"Rectangle", "Pen", "Oval", "Eraser"};
	
	/*
		Native functions : 프로젝트에서 사용하는 native 함수들의 정의
	*/
	public native int openSwitch();
	public native int closeSwitch();
	public native int readSwitch(int fd);
	public native int openDot(int value);
	public native int openFND(String str);
	public native int openLCD(String str);
	public native void closeLCD();
	public native void closeFND();
	public native void closeDOT();
	public native void writeLCDFirstLine(String str);
	public native void writeLCDSecondLine(int mode, int x1, int y1, int x2, int y2);
	public native int closeDot(int fd);
	public native void startPushSwitch();
	public void getSwitchValues(int[] values){
		//Log.d("FpgaPushSwitch", "Push Switch Values: " + Arrays.toString(values));

		//Native 함수를 통해 values의 값에 현재 push switch에서 읽은 값을 저장 후, 변화가 있었을 경우 각 작동을 tigger
        if(values[0] == 1)
        	undoFlag = 1;
        else if(values[1] == 1)
        	redoFlag = 1;
        else if(values[4] == 1)
        	incStrokeFlag = 1;
        else if(values[3] == 1)
        	decStrokeFlag = 1;
        else if(values[8] == 1){
        	resetFlag = 1;
        	//finish();
        }

		//push switch의 값을 읽어서 특정 버튼이 눌러진 경우 background thread를 통해 paintView에서의 작동을 실행시키고 mainUI에서
		//갱신을 진행한다. 
        if(undoFlag == 1) {
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    paintView.undoFromMain();
                    undoFlag = 0;
                }
            });
        }
        
        if(redoFlag == 1){
        	runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    paintView.redoFromMain();
                    redoFlag = 0;
                }
            });
        }
        
        if(incStrokeFlag == 1){
        	runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    paintView.incStrokeSize();
                    incStrokeFlag = 0;
                }
            });
        }
        
        if(decStrokeFlag == 1){
        	runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    paintView.decStrokeSize();
                    decStrokeFlag = 0;
                }
            });
        }
        
        if(resetFlag == 1){
        	runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    paintView.resetFromMain();
                    resetFlag = 0;
                }
            });
        }
	};
	
	boolean opened = false;
	private PaintView paintView;
	private TextView ColorDisplay;
	private TextView test, input;
	int fpga_dot, fpga_fnd, fpga_lcd, fpga_switch, curMode = 1;
	float x1, y1, x2, y2;
	int switch_value, num_pressed = 0;
	
	/*
	 onCreate() : called when application is created
	 */
	@Override
	protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

		//실행되는 application의 window 정의
		requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.activity_main);
        
		//새 PaintView 생성
		paintView = new PaintView(this);
		
		//layout에서 정의한 내용들이 application 실행 화면에 출력될 수 있도록 각 요소들을 생성
		ColorDisplay = (TextView)findViewById(R.id.colorDisplay);
		LinearLayout container = (LinearLayout)findViewById(R.id.container);
    	LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.MATCH_PARENT);
    	container.addView(paintView, params);
		Button ColorPicker = (Button)findViewById(R.id.ColorPicker);
		Button Mode = (Button)findViewById(R.id.Mode);
		Button undo = (Button)findViewById(R.id.undo);
		Button redo = (Button)findViewById(R.id.redo);
		input = (TextView)findViewById(R.id.number);
		undo = (Button)findViewById(R.id.undo);
		redo = (Button)findViewById(R.id.redo);
		paintView.SetTextView(ColorDisplay, input);
		paintView.SetButtons(ColorPicker, undo, redo, Mode);
		paintView.SetButtonListeners();
		openDot(1);
		openFND("0000");
		openLCD("Pen                             ");
		//input.setText("dd");
		
        // 리스너 설정. 그린 횟수가 변경되는 경우 호출되는 listener로 FND의 값을 갱신할 수 있도록 JNI를 통해 관련 함수를 호출한다. 
        paintView.setOnNumberChangeListener(new PaintView.OnNumberChangeListener() {
            @Override
            public void onNumberChanged(int newNumber) {
            	String s;
            	if(newNumber < 10){
            		s = "000";
            	}
            	else if(newNumber < 100)
            		s = "00";
            	else if(newNumber < 1000)
            		s="0";
            	else
            		s="";
            	openFND(s + newNumber);
            }
        });
        
        // 리스너 설정. Pen/Eraser mode일 때 stroke size의 값이 변경되는 listener로 DOT의 값을 갱신할 수 있도록 관련 함수를 호출한다. 
        paintView.setOnStrokeChangeListener(new PaintView.OnNumberChangeListener() {
            @Override
            public void onNumberChanged(int newNumber) {
            	openDot(newNumber);
            }
        });
        
     // 리스너 설정. 모드 변경시 호출되는 listener로 LCD와 DOT의 값이 갱신될 수 있도록 관련 함수를 호출한다. 
        paintView.setOnModeChangeListener(new PaintView.OnNumberChangeListener() {
            @Override
            public void onNumberChanged(int newNumber) {
            	writeLCDFirstLine(modeArr[newNumber]);
            	curMode = newNumber;
            	if(curMode == 1 || curMode == 3){
            		openDot(1);
            	}
            	else
            		openDot(-1);
            }
        });
       
        //touch가 끝났을 때 호출되는 listener로 LCD의 값을 갱신할 수 있도록 관련 함수를 호출한다. 
        paintView.setOnCoordinatesChangeListener(new PaintView.OnCoordinatesChangeListener() {
        	@Override
            public void onCoordinatesChanged(float x1, float y1, float x2, float y2) {
            	writeLCDSecondLine(curMode, Math.round(x1), Math.round(y1), Math.round(x2), Math.round(y2));
            }
        });
        
        //push switch에 입력이 주어지는지 일정 시간마다 체크하기 위해 개별 thread로 관리. 
        new Thread(new Runnable() {
            @Override
            public void run() {
                startPushSwitch();
            }
        }).start();
	}
	
	/*
	onStart() : called when application is started.
	 */
	@Override
	protected void onStart(){
		super.onStart();
		
		
	}
	
	@Override
	protected void onDestroy(){
		super.onDestroy();
		Log.d("onDestroy", "Finish called\n");
	}
}
