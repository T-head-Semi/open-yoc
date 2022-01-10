<template>
  <demo-scroller>
    <div class="main-wrapper">
      <div class="section">
        <text class="section-title">fill&stroke</text>
        <canvas ref="c1" class="canvas" width="500" height="120"></canvas>
      </div>

      <div class="section">
        <text class="section-title">gridient</text>
        <canvas ref="c2" class="canvas" width="500" height="120"></canvas>
      </div>

      <div class="section">
        <text class="section-title">path</text>
        <canvas ref="c3" class="canvas" width="500" height="120"></canvas>
      </div>

      <div class="section">
        <text class="section-title">Line style</text>
        <canvas ref="c4" class="canvas" width="500" height="120"></canvas>
      </div>
    </div>
  </demo-scroller>
</template>

<script>
export default {
  name: "canvas",
  props: {},
  data() {
    return {};
  },
  mounted() {
    this.c1();
    this.c2();
    this.c3();
    this.c4();
  },
  methods: {
    hslToHex(h, s, l) {
      l /= 100;
      const a = s * Math.min(l, 1 - l) / 100;
      const f = n => {
        const k = (n + h / 30) % 12;
        const color = l - a * Math.max(Math.min(k - 3, 9 - k, 1), -1);
        return Math.round(255 * color).toString(16).padStart(2, '0');   // convert to Hex and prefix "0" if needed
      };
      return `#${f(0)}${f(8)}${f(4)}`;
    },

    c1() {
      let ctx = typeof createCanvasContext === "function" ? createCanvasContext(this.$refs.c1) : this.$refs.c1.getContext("2d");
      ctx.fillStyle = "#FF6A00";
      ctx.fillRect(0, 0, 200, 110);
      ctx.strokeStyle = "#FF6A00";
      ctx.lineWidth = 6;
      ctx.strokeRect(212, 3, 194, 104);
    },

    c2() {
      let ctx = typeof createCanvasContext === "function" ? createCanvasContext(this.$refs.c2) : this.$refs.c2.getContext("2d");

      ctx.save()
      let grad = ctx.createLinearGradient(0, 0, 200, 0);

      grad.addColorStop(0, "#FF0000");
      grad.addColorStop(0.17, "#FF00FF");
      grad.addColorStop(0.25, "#8000FF");
      grad.addColorStop(0.33, "#0000FF");
      grad.addColorStop(0.42, "#0080FF");
      grad.addColorStop(0.5, "#00FFFF");
      grad.addColorStop(0.67, "#00FF00");
      grad.addColorStop(0.75, "#80FF00");
      grad.addColorStop(0.79, "#BEFF00");
      grad.addColorStop(0.83, "#FFFF00");
      grad.addColorStop(0.92, "#FF8000");
      grad.addColorStop(1, "#FFBF00");
      ctx.fillStyle = grad;
      ctx.fillRect(0, 0, 200, 110);
      ctx.restore()

      ctx.translate(220, 0)
      this.drawColorPan(ctx, 120, 120)

    },
    c3() {
      let ctx = typeof createCanvasContext === "function" ? createCanvasContext(this.$refs.c3) : this.$refs.c3.getContext("2d");

      ctx.save()
      // draw mickey
      ctx.fillStyle = "#FF6A00";

      ctx.beginPath(); ctx.arc(70, 65, 45, 0, 2 * Math.PI); ctx.fill();
      ctx.beginPath(); ctx.arc(25, 25, 25, 0, 2 * Math.PI); ctx.fill();
      ctx.beginPath(); ctx.arc(110, 25, 25, 0, 2 * Math.PI); ctx.fill();

      ctx.restore()

      ctx.translate(220, 0)
      // draw star, refer: https://www.programmersought.com/article/74144516966/
      ctx.save()
      ctx.fillStyle = "#FF6A00";
      ctx.beginPath()
      // Move the coordinates to the center of the canvas
      ctx.translate(60, 60)
      // Set the radius of big circle and small circle
      let R = 60
      let r = 30
      let x
      let y
      // Calculate the coordinates of the 10 points around the five-pointed star counterclockwise
      for(let i = 0; i <= 5; i++){
        // The coordinates of each point protruding from the periphery
        x = Math.cos((18 + 72*i) / 180 * Math.PI) * R
        y = - Math.sin((18 + 72*i) / 180 * Math.PI) * R // The positive direction of the y-axis in the canvas is opposite to the rectangular coordinate system
        ctx.lineTo(x, y)
        // The coordinates of each point where the periphery is recessed
        x = Math.cos((54 + 72*i) / 180 * Math.PI) * r
        y = - Math.sin((54 + 72*i) / 180 * Math.PI) * r // The positive direction of the y-axis in the canvas is opposite to the rectangular coordinate system
        ctx.lineTo(x, y)
      }
      ctx.closePath()
      ctx.fill()
      ctx.restore()
    },
    c4() {
      let ctx = typeof createCanvasContext === "function" ? createCanvasContext(this.$refs.c4) : this.$refs.c4.getContext("2d");

      ctx.save()
      ctx.beginPath();
      ctx.translate(10, 2);
      ctx.lineWidth = 3;
      ctx.strokeStyle = "#FF6A00";
      this.drawSin(ctx);
      ctx.restore();

      ctx.save()
      ctx.beginPath();
      ctx.translate(250, 5);
      ctx.strokeStyle = "#FF6A00";
      ctx.lineWidth = 10;
      ctx.lineJoin = "round";
      ctx.lineCap = "round";
      this.drawSin(ctx);
      ctx.restore()

    },
    drawSin(ctx, deltax, deltay) {
      ctx.beginPath();
      let amplitude = 50 // top to 0 of amplitude
      let wavelen = 50  // 180 degree

      let counter = 0,
        x = 0,
        y = amplitude;
      ctx.moveTo(x, y);

      let drawPeriod = 720;
      let increase = 90 / 180 * Math.PI / 9;
      for (let i = 0; i <= drawPeriod; i = i + 10) {
        x = i/drawPeriod * wavelen*(drawPeriod/180);
        y = amplitude - Math.sin(counter) * wavelen;
        counter += increase;
        ctx.lineTo(x, y);
      }
      ctx.stroke();
    },

    drawColorPan(ctx, width, height) {
      let radius = 60,
          thickness = 60,
          p = {
              x: width,
              y: height,
          },
          start = 0,
          end = 2* Math.PI,
          step = Math.PI / 360,
          ang = 0,
          grad;
      ctx.save()
      ctx.translate(p.x/2, p.y/2);

      for (ang = start; ang <= end; ang += step) {
          ctx.save();
          ctx.rotate(-ang);
          grad = ctx.createLinearGradient(0, radius - thickness, 0, radius);

          let h = 360-(ang-start)/(end-start) * 360;

          grad.addColorStop(0, this.hslToHex(h, 100, 50));

          grad.addColorStop(1, this.hslToHex(h, 100, 50));
          ctx.fillStyle = grad;

          ctx.fillRect(0, radius - thickness, radius/50, thickness);
          ctx.restore();
      }
      ctx.restore()
    },
  },
};
</script>

<style lang="less" scoped>
@import "base.less";
.canvas {
  align-self: flex-start;
  width:500px;
  height:120px;
}
</style>
