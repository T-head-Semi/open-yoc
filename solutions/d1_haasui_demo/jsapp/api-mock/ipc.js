/*
 * ipc接口mock
 */

function service(sid) {
  console.log('ipc service')
  return {
    request(){
      console.log('ipc request:', arguments);
    },
    on(callback){
      console.log('ipc on:', callback);
    }
  }
}

export {
  service
};