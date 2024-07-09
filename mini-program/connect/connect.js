// pages/connect/connect.js
const chooseLocation = requirePlugin('chooseLocation');
Page({
  radioChange: function(e) {
    console.log('radio发生change事件，携带value值为：', e.detail.value);
   this.setData({
     mode: e.detail.value
   })
  },
  _pickEndAddress: function(e) {
    // 在这个函数里写给endPoint赋值的语句
    var that = this;
    that.location_state = "pickEndAddress";
    console.log('选取地址终点', e);
    let latitude = that.data.location.latitude;
    let longitude = that.data.location.longitude;
    console.log("latitude", latitude);
    console.log("longitude", longitude);
    this.getUserLocation();
    const key = 'LHBBZ-RQO6L-NP2PJ-MHHT5-SZDXS-W2FIQ'; //使用在腾讯位置服务申请的key
    const referer = '盲人辅助导航'; //调用插件的app的名称
    const location = JSON.stringify({
      latitude: latitude,
      longitude: longitude
    });
    const category = '生活服务,娱乐休闲';


    wx.navigateTo({
      url: 'plugin://chooseLocation/index?key=' + key + '&referer=' + referer + '&location=' + location + '&category=' + category
    });

  },

  _pickStartAddress: function(e) {
    var that = this;
    that.location_state = "pickStartAddress";
    console.log('选取地址', e);
    let latitude = that.data.location.latitude;
    let longitude = that.data.location.longitude;
    console.log("latitude", latitude);
    console.log("longitude", longitude);
    this.getUserLocation();
    const key = 'LHBBZ-RQO6L-NP2PJ-MHHT5-SZDXS-W2FIQ'; //使用在腾讯位置服务申请的key
    const referer = '盲人辅助导航'; //调用插件的app的名称
    const location = JSON.stringify({
      latitude: latitude,
      longitude: longitude
    });
    const category = '生活服务,娱乐休闲';


    wx.navigateTo({
      url: 'plugin://chooseLocation/index?key=' + key + '&referer=' + referer + '&location=' + location + '&category=' + category
    });

  },

  getUserLocation: function() {
    let vm = this
    wx.getSetting({
      success: (res) => {
        // res.authSetting['scope.userLocation'] == undefined  表示 初始化进入该页面
        // res.authSetting['scope.userLocation'] == false  表示 非初始化进入该页面,且未授权
        // res.authSetting['scope.userLocation'] == true  表示 地理位置授权
        // 拒绝授权后再次进入重新授权
        console.log("授权与否", res.authSetting['scope.userLocation'])
        if (res.authSetting['scope.userLocation'] != undefined && res.authSetting['scope.userLocation'] != true) {
          // console.log('authSetting:status:拒绝授权后再次进入重新授权', res.authSetting['scope.userLocation'])
          wx.showModal({
            title: '',
            content: '【盲人辅助导航】需要获取你的地理位置，请确认授权',
            success: function(res) {
              if (res.cancel) {
                wx.showToast({
                  title: '拒绝授权',
                  icon: 'none'
                })
                setTimeout(() => {
                  wx.navigateBack()
                }, 1500)
              } else if (res.confirm) {
                wx.openSetting({
                  success: function(dataAu) {
                    // console.log('dataAu:success', dataAu)
                    if (dataAu.authSetting["scope.userLocation"] == true) {
                      //再次授权，调用wx.getLocation的API
                      vm.getLocation(dataAu)
                    } else {
                      wx.showToast({
                        title: '授权失败',
                        icon: 'none'
                      })
                      setTimeout(() => {
                        wx.navigateBack()
                      }, 1500)
                    }
                  }
                })
              }
            }
          })
        }
        // 初始化进入，未授权
        else if (res.authSetting['scope.userLocation'] == undefined) {
          // console.log('authSetting:status:初始化进入，未授权', res.authSetting['scope.userLocation'])
          //调用wx.getLocation的API
          vm.getLocation(res)
        }
        // 已授权
        else if (res.authSetting['scope.userLocation']) {
          // console.log('authSetting:status:已授权', res.authSetting['scope.userLocation'])
          //调用wx.getLocation的API
          vm.getLocation(res)
        }
      }
    })
  },
  // 微信获得经纬度
  getLocation: function(userLocation) {
    let vm = this
    wx.getLocation({
      type: "wgs84",
      success: function(res) {
        // console.log('getLocation:success', res)
        var latitude = res.latitude
        var longitude = res.longitude
        // vm.getDaiShu(latitude, longitude)
        console.log('经纬度', latitude, longitude)
        vm.setData({
          location: {
            latitude: latitude,
            longitude: longitude
          }
        })
      },
      fail: function(res) {
        // console.log('getLocation:fail', res)
        if (res.errMsg === 'getLocation:fail:auth denied') {
          wx.showToast({
            title: '拒绝授权',
            icon: 'none'
          })
          setTimeout(() => {
            wx.navigateBack()
          }, 1500)
          return
        }
        if (!userLocation || !userLocation.authSetting['scope.userLocation']) {
          vm.getUserLocation()
        } else if (userLocation.authSetting['scope.userLocation']) {
          wx.showModal({
            title: '',
            content: '请在系统设置中打开定位服务',
            showCancel: false,
            success: result => {
              if (result.confirm) {
                wx.navigateBack()
              }
            }
          })
        } else {
          wx.showToast({
            title: '授权失败',
            icon: 'none'
          })
          setTimeout(() => {
            wx.navigateBack()
          }, 1500)
        }
      }
    })
  },

  _handleSubmit: function(e) {

    console.log("开始导航", e);
    let plugin = requirePlugin('routePlan');
    const key = 'LHBBZ-RQO6L-NP2PJ-MHHT5-SZDXS-W2FIQ'; //使用在腾讯位置服务申请的key
    const referer = '盲人辅助导航'; //调用插件的app的名称
    let endPoint = this.data.endAddressInfo;
    endPoint = JSON.stringify(endPoint);
    console.log("==endPoint---", endPoint);
    //let mode=this.data.mode;
    let startPoint = this.data.startAddressInfo;
    startPoint = JSON.stringify(startPoint);
    console.log("==startPoint---", startPoint);
    let mode=this.data.mode;
    if (startPoint=='null') {
      console.log("进来了");
      wx.navigateTo({
        url: 'plugin://routePlan/index?key=' + key + '&referer=' + referer + '&endPoint=' + endPoint + '&navigation=1' + '&mode=' + mode
      })
    } else {
      wx.navigateTo({
        url: 'plugin://routePlan/index?key=' + key + '&referer=' + referer + '&endPoint=' + endPoint + '&startPoint=' + startPoint + '&navigation=1'+'&mode=' + mode
      });
    }
  },
  /**
   * 页面的初始数据
   */
  data: {
    location: {},
    startAddressInfo: {},
    endAddressInfo: {

    },
    mode:"driving"
  },

  /**
   * 生命周期函数--监听页面加载
   */
  onLoad: function(options) {

  },

  /**
   * 生命周期函数--监听页面初次渲染完成
   */
  onReady: function() {

  },

  /**
   * 生命周期函数--监听页面显示
   */
  onShow: function() {
    this.getUserLocation();
    const location = chooseLocation.getLocation();
    console.log(location)
    if(this.location_state == "pickStartAddress")
    {
      this.setData({
        startAddressInfo: location
      })
    }
    else if(this.location_state == "pickEndAddress"){
      this.setData({
        endAddressInfo: location
      })
    }
    
  },

  /**
   * 生命周期函数--监听页面隐藏
   */
  onHide: function() {

  },

  /**
   * 生命周期函数--监听页面卸载
   */
  onUnload: function() {

  },

  /**
   * 页面相关事件处理函数--监听用户下拉动作
   */
  onPullDownRefresh: function() {

  },

  /**
   * 页面上拉触底事件的处理函数
   */
  onReachBottom: function() {

  },

  /**
   * 用户点击右上角分享
   */
  onShareAppMessage: function() {

  }
})