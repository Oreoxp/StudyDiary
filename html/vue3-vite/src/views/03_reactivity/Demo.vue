<script setup>
    import {ref,reactive} from 'vue'

    const state = reactive({count:0})
    const count = ref(0)
    
    const increment = () =>{
        state.count++;
    }
    const add = () =>{
        count.value++;
    }
</script>

<template>
    <div>
        <button @click="increment">
            {{state.count}}
        </button>
        <button @click="add">
            {{count}}
        </button>
    </div>
</template>

<style lang="css">
    
</style>
