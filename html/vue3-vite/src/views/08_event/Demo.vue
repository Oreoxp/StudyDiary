<script setup>
    import {ref} from 'vue'
    const count =ref(0)
    
    const handleclick = (c) =>{
        count.value += c
    }

</script>

<template>
    <div>
        {{count}}
    </div>
    <div>
        <button v-on:click="handleclick(1)">
            add
        </button>
    </div>
</template>

<style lang="css">
    
</style>
