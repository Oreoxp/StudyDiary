<script setup>
    import {ref} from 'vue'
    const hasColor = ref(true)
    const colorlist = ref(["001","002","003"])
    const curindex = ref(0)

    const change = ()=>{
        hasColor.value = !hasColor.value
    }
    const clickli = (index) =>{
        curindex.value = index
    }
</script>

<template>
    <div :class="{a:hasColor}">
        <p> hello </p>
    </div>
    <div>
        <button @click="change"> 
            change
        </button>
    </div>
    <ul>
        <li 
            v-for="it,index in colorlist" 
            :class="{active: curindex === index}" 
            @click="clickli(index)"
        >
            {{ it }}
        </li>
    </ul>
</template>

<style lang="css">
        .active{
            color: green;
            font-size: 30px;
        }
</style>
