<script setup>
    import {ref,computed} from 'vue'
    const count = ref(1)

    const handleclick = ()=>{
        count.value++;
    }
    const doubleclick = computed(()=>{
        return count.value*2;
    })
</script>

<template>
    <div>
        {{ count }}
        {{ doubleclick}}
    </div>
    <div>
        <button @click="handleclick">add</button>
    </div>
</template>

<style lang="css">
    
</style>
