const app = getApp();
//引入插件：微信同声传译
const plugin = requirePlugin('WechatSI');
Page({
  data: {
    userInput: 5000 ,// 默认值设置为5000
    shuju: '',
    content: '',//内容
    src:'', //
  },
  onReady(e) {
    //创建内部 audio 上下文 InnerAudioContext 对象。
    this.innerAudioContext = wx.createInnerAudioContext();
    this.innerAudioContext.onError(function (res) {
      console.log(res);
      wx.showToast({
        title: '语音播放失败',
        icon: 'none',
      })
    }) 
  },
  // conInput: function (e) {
  //   this.setData({
  //     content: shuju,
  //   })
  // },
  wordYun:function (e) {
    var that = this;
    var content = this.data.content;
    plugin.textToSpeech({
      lang: "zh_CN",
      tts: true,
      content: content,
      success: function (res) {
        console.log(res);
        console.log("succ tts", res.filename);
        that.setData({
          src: res.filename
        })
        that.yuyinPlay();
 
      },
      fail: function (res) {
        console.log("fail tts", res)
      }
    })
  },
  yuyinPlay: function (e) {
    if (this.data.src == '') {
      console.log(暂无语音);
      return;
    }
    this.innerAudioContext.src = this.data.src //设置音频地址
    this.innerAudioContext.play(); //播放音频
  },
 
  // 结束语音
  end: function (e) {
    this.innerAudioContext.pause();//暂停音频
  },

  	/**
     * 获取token
     */ 
        getshadow:function(){
        console.log("开始获取影子");//打印完整消息
        var that=this;  //这个很重要，在下面的回调函数中由于异步问题不能有效修改变量，需要用that获取
        var token=wx.getStorageSync('token');//读缓存中保存的token
        console.log("我的toekn:"+token);//打印完整消息
        wx.request({
            url: 'https://cc3c5cd671.st1.iotda-app.cn-north-4.myhuaweicloud.com:443/v5/iot/04153c6940d64f699f61cef74e958acc/devices/662a542771d845632a06fe33_hi3861/shadow',
            data:'',
            method: 'GET', // OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE, CONNECT
            header: {'content-type': 'application/json','X-Auth-Token':token }, //请求的header 
            success: function(res){// success
              // success
                console.log(res);//打印完整消息
                var shadow=JSON.stringify(res.data.shadow[0].reported.properties);
                var shuju=JSON.stringify(res.data.shadow[0].reported.properties.AutoModule);
                console.log('设备影子数据：'+shadow);
                console.log(''+shuju);
                that.setData({
                  content: shuju, // shuju是想要转换为语音的字符串
                  shuju: shuju // 更新shuju
                }, function () {
                  // 在setData的回调中调用播放语音的方法
                  that.wordYun(); // 获取到shuju后立即调用wordYun播放语音
                });
                //以下根据自己的设备属性进行解析
                //我的设备影子：{"Temp":30,"temp":89,"Dev_data":77.20592,"humi":80,"light":3000,"GPS_N":3904.2279,"GPS_E":11706.2279,"warning":1,"MPU6050":1,"foot":12,"led":1,"temps":"89"}
                // var Temp=JSON.stringify(res.data.shadow[0].reported.properties.Key);
                // var Humi=JSON.stringify(res.data.shadow[0].reported.properties.value);
                // console.log('温度='+Temp+'℃');
                // console.log('湿度='+Humi+'%');
                // that.setData({result:'温度'+Temp+'℃,湿度'+Humi+'%'});

            },
            fail:function(){
                // fail
                console.log("获取影子失败");//打印完整消息
            },
            complete: function() {
                // complete
                console.log("获取影子完成");//打印完整消息
            } 
        });
    },

    gettoken:function(){
      console.log("开始获取。。。");//打印完整消息
      var that=this;  //这个很重要，在下面的回调函数中由于异步问题不能有效修改变量，需要用that获取
      wx.request({
          url: 'https://iam.cn-north-4.myhuaweicloud.com/v3/auth/tokens',
          data:'{"auth": { "identity": {"methods": ["password"],"password": {"user": {"name": "ZhangYugeng","password": "qpwoei123","domain": {"name": "GT-tyhbvf"}}}},"scope": {"project": {"name": "cn-north-4"}}}}',
          method: 'POST', // OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE, CONNECT
          header: {'content-type': 'application/json' }, // 请求的 header 
          success: function(res){// success
            // success
              console.log("获取token成功");//打印完整消息
              console.log(res);//打印完整消息
              var token='';
              token=JSON.stringify(res.header['X-Subject-Token']);//解析消息头token
              token=token.replaceAll("\"", "");
              console.log("获取token=\n"+token);//打印token
              wx.setStorageSync('token',token);//把token写到缓存中,以便可以随时随地调用

          },
          fail:function(){
              // fail
              console.log("获取token失败");//打印完整消息
          },
          complete: function() {
              // complete
              console.log("获取token完成");//打印完整消息
          } 
      });
  },

     /**
     * 页面的初始数据
     */
    // data: {
    //     result:'等待获取token',
    // },
    /**
     * 获取token按钮按下：
     */
    touchBtn_gettoken:function()
    {
        console.log("获取token按钮按下");
        this.gettoken();
    },
    /**
     * 获取设备影子按钮按下：
     */
    touchBtn_getshadow:function()
    {
        console.log("单点自控按钮按下");
        this.gettoken();
        this.getshadow();
        
    },

    select_mode: function() 
    {
      var that = this;
    wx.showActionSheet({
      itemList: ['高速', '中速', '低速'],
      success: function(res) {
        if (res.tapIndex === 0) {
          // 用户选择了高速
          that.setData({
            userInput: 3000
          });
        } else if (res.tapIndex === 1) {
          // 用户选择了中速
          that.setData({
            userInput: 6000
          });
        } else if (res.tapIndex === 2) {
          // 用户选择了低速
          that.setData({
            userInput: 10000
          });
        }
      }
    });
    },

    onInputValue: function(e) {
      // 获取用户输入的值
      const value = e.detail.value;
      // 判断用户是否输入了内容
      if (value) {
        // 如果有输入，则解析为浮点数
        const inputValue = parseFloat(value);
        // 判断输入值是否大于等于3000
        if (inputValue >= 3000) {
          // 如果输入值大于等于3000，则更新数据变量
          this.setData({
            userInput: inputValue
          });
        } else {
          // 如果输入值小于3000，则使用默认值5000
          this.setData({
            userInput: 5000
          });
          wx.vibrateShort();
        }
      } else {
        // 如果没有输入，则使用默认值
        this.setData({
          userInput: 5000
        });
      }
    },

    start_work:function()
    {
      console.log("开始识障工作");
      this.gettoken();
      // 启动定时器
    this.timer = setInterval(() => {
      this.getshadow();
    }, this.data.userInput); // 单位ms自设延迟时间
    },
    end_work: function() {
      // 页面卸载时清除定时器
      clearInterval(this.timer);
      console.log("结束识障工作");
    },

  });
