<template>
  <div>
    <fl-button @click="ipcTest">ipcTest</fl-button>
    <fl-button @click="ipcPayload">ipcPayload</fl-button>
  </div>
</template>
<script>
import Ipc from "@/libs/ipc.js";
import { FlButton } from "falcon-ui";

export default {
  components: { FlButton },
  data() {
    return {};
  },
  methods: {
    ipcTest() {
      const service = new Ipc(1110);


      try{
        service.request(1, 2);
      } catch(e){
        console.log(e);
      }
      service.finalize();
    },

    ipcPayload() {
      const service = new Ipc(1110);
      const payload = new Ipc.Parcel()
        .writeString("aaa吃饭aabb吃饭cc")
        .writeInt32(12)
        .writeUint32(123)
        .writeFloat(1.1)
        .writeDouble(1.2)
        .writeByteArray();

      service.request({
        what: 3,
        arg: 4,
        payload: payload.getData()
      });

      service.finalize();
    }
  }
};
</script>
<style scoped>
</style>
