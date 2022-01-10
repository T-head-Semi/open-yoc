/*
 * jsapi扩展mock
 */
export default {
  playByLib(params) {
    console.log('mock playByLib', JSON.stringify(params));
  },
  playByRtp(params){
    console.log('mock playByRtp', JSON.stringify(params));
  },
  playByCustom(params){
    console.log('mock playByCustom', JSON.stringify(params));
  }
}