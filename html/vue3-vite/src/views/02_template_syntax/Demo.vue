<script setup>
    import {ref,reactive} from 'vue'
    const msg = ref('test')
    const rawHtml = ref('<h1>raw test</h1>')

    const title = ref('test2')
    const obj = ref({
        id:'001',
        class:'box'
    })

    const num = ref(100)
    const words = ref("hello")
    const isShow = ref(true)
</script>

<template>
    <div>
        {{ msg }}
    </div>
    <div>
        {{ rawHtml }}
    </div>
    <div v-html="rawHtml">
    </div>
    <div v-bind:title="title">title test</div>
    <br>
    <div :="obj">obj</div>

    <div>
        {{ num +1 }}
        {{ words.split('').reverse().join('') }}
    </div>

    <div v-if="isShow">
        1
    </div>
</template>

<style lang="css">
    
</style>
